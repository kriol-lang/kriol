import { test } from 'node:test';
import assert from 'node:assert/strict';
import { chmodSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { compareWithCompiler, readCompilerWords } from '../src/compiler.js';
import { loadSpec } from '../src/spec.js';

// What `kriol --print-words` prints, for a small language.
const COMPILER_WORDS = {
  keywords: ['fn', 'si'],
  builtinTypes: ['int', 'i8'],
  booleans: ['sin'],
  builtinFunctions: ['mostra'],
  preludeTypes: ['Erru'],
  deprecatedWords: {},
};

function specWith(words, deprecatedWords = {}) {
  return {
    deprecatedWords,
    words: {
      declarationKeywords: ['fn'],
      controlKeywords: ['si'],
      builtinTypes: ['int', 'i8'],
      booleans: ['sin'],
      builtinFunctions: ['mostra'],
      preludeTypes: ['Erru'],
      ...words,
    },
  };
}

test('accepts an old keyword spelling that both deprecate', () => {
  const compiler = {
    ...COMPILER_WORDS,
    keywords: ['fn', 'si', 'sinon'],
    deprecatedWords: { sinon: 'si' },
  };

  assert.deepEqual(compareWithCompiler(specWith({}, { sinon: 'si' }), compiler), []);
});

test('reports a deprecated word the specification lacks', () => {
  const compiler = { ...COMPILER_WORDS, deprecatedWords: { mostran: 'mostra' } };

  assert.deepEqual(compareWithCompiler(specWith({}), compiler), [
    "deprecatedWords: 'mostran -> mostra' is in the compiler but not in the specification",
  ]);
});

test('finds no difference when the specification matches the compiler', () => {
  assert.deepEqual(compareWithCompiler(specWith({}), COMPILER_WORDS), []);
});

test('reports a keyword the compiler has and the specification lacks', () => {
  const spec = specWith({ controlKeywords: [] });

  assert.deepEqual(compareWithCompiler(spec, COMPILER_WORDS), [
    "keywords: 'si' is in the compiler but not in the specification",
  ]);
});

test('reports a builtin function the compiler does not know', () => {
  const spec = specWith({ builtinFunctions: ['mostra', 'skribi'] });

  assert.deepEqual(compareWithCompiler(spec, COMPILER_WORDS), [
    "builtinFunctions: 'skribi' is in the specification but not in the compiler",
  ]);
});

test('reads the words a compiler prints with --print-words', { skip: process.platform === 'win32' && 'needs a POSIX shell' }, () => {
  const dir = mkdtempSync(join(tmpdir(), 'kriol-words-'));
  const kriol = join(dir, 'kriol');
  writeFileSync(kriol, `#!/bin/sh\n[ "$1" = --print-words ] && echo '${JSON.stringify(COMPILER_WORDS)}'\n`);
  chmodSync(kriol, 0o755);
  try {
    assert.deepEqual(readCompilerWords(kriol), COMPILER_WORDS);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

// KRIOL names a built compiler; run_tests.sh sets it to the one under test.
const kriol = process.env.KRIOL;

test('the specification matches the words the compiler prints', { skip: !kriol && 'set KRIOL to a built kriol binary' }, () => {
  assert.deepEqual(compareWithCompiler(loadSpec(), readCompilerWords(kriol)), []);
});
