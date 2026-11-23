// Usage: node scripts/pack.js <release-tag> <path/to/kriol> <out-dir>
//
// Writes the release files of the package into <out-dir>:
//   kriol-syntax-<version>.tgz   the npm package, installed by URL
//   kriol.tmLanguage.json        the TextMate grammar, for tools without npm

import { execFileSync } from 'node:child_process';
import { copyFileSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { buildArtifacts } from '../src/build.js';
import { releaseVersion } from '../src/release.js';
import { loadSpec, validateSpec } from '../src/spec.js';

const [tag, kriol, outArg] = process.argv.slice(2);
if (!tag || !kriol || !outArg) {
  console.error('Usage: node scripts/pack.js <release-tag> <path/to/kriol> <out-dir>');
  process.exit(1);
}

const packageDir = new URL('../', import.meta.url);
const outDir = resolve(outArg);

function fail(message) {
  console.error(message);
  process.exit(1);
}

function stagePackage(version, spec) {
  const stage = mkdtempSync(join(tmpdir(), 'kriol-syntax-'));
  const manifest = JSON.parse(readFileSync(new URL('package.json', packageDir), 'utf8'));
  writeFileSync(join(stage, 'package.json'), `${JSON.stringify({ ...manifest, version }, null, 2)}\n`);

  copyFileSync(new URL('kriol-syntax.json', packageDir), join(stage, 'kriol-syntax.json'));
  copyFileSync(new URL('README.md', packageDir), join(stage, 'README.md'));
  copyFileSync(new URL('../LICENSE', packageDir), join(stage, 'LICENSE'));

  mkdirSync(join(stage, 'dist'));
  for (const [file, content] of Object.entries(buildArtifacts(spec)))
    writeFileSync(join(stage, 'dist', file), content);
  return stage;
}

let version;
try {
  version = releaseVersion(tag, execFileSync(kriol, ['--version'], { encoding: 'utf8' }));
} catch (error) {
  fail(error.message);
}

const spec = loadSpec();
const problems = validateSpec(spec);
if (problems.length > 0) fail(`kriol-syntax.json is invalid:\n  ${problems.join('\n  ')}`);

const stage = stagePackage(version, spec);
try {
  mkdirSync(outDir, { recursive: true });
  execFileSync('npm', ['pack', '--pack-destination', outDir], { cwd: stage, stdio: ['ignore', 'ignore', 'inherit'] });
  copyFileSync(join(stage, 'dist', 'kriol.tmLanguage.json'), join(outDir, 'kriol.tmLanguage.json'));
} finally {
  rmSync(stage, { recursive: true, force: true });
}

console.log(`wrote kriol-syntax-${version}.tgz and kriol.tmLanguage.json to ${outDir}`);
