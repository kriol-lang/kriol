// Usage: node scripts/check-compiler.js <path/to/kriol>

import { compareWithCompiler, readCompilerWords } from '../src/compiler.js';
import { loadSpec } from '../src/spec.js';

const kriol = process.argv[2];
if (!kriol) {
  console.error('Usage: node scripts/check-compiler.js <path/to/kriol>');
  process.exit(1);
}

const problems = compareWithCompiler(loadSpec(), readCompilerWords(kriol));

if (problems.length > 0) {
  console.error(`kriol-syntax.json differs from the words ${kriol} prints:\n  ${problems.join('\n  ')}`);
  process.exit(1);
}
console.log(`kriol-syntax.json matches the words ${kriol} prints`);
