import { mkdirSync, writeFileSync } from 'node:fs';
import { buildArtifacts, DIST_DIR } from '../src/build.js';
import { loadSpec, validateSpec } from '../src/spec.js';

const spec = loadSpec();
const problems = validateSpec(spec);
if (problems.length > 0) {
  console.error(`kriol-syntax.json is invalid:\n  ${problems.join('\n  ')}`);
  process.exit(1);
}

mkdirSync(DIST_DIR, { recursive: true });
for (const [file, content] of Object.entries(buildArtifacts(spec))) {
  writeFileSync(new URL(file, DIST_DIR), content);
  console.log(`wrote dist/${file}`);
}
