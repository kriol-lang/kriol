import { test } from 'node:test';
import assert from 'node:assert/strict';
import { WORD_CATEGORIES, allWords, highlightedWords, loadSpec, validateSpec } from '../src/spec.js';

function validSpec() {
  return {
    name: 'kriol',
    displayName: 'Kriol',
    scopeName: 'source.kriol',
    fileExtensions: ['kriol', 'kr'],
    comments: { line: '//', block: ['/*', '*/'] },
    words: Object.fromEntries(WORD_CATEGORIES.map((category, i) => [category, [`w${i}`]])),
  };
}

test('the specification is valid', () => {
  assert.deepEqual(validateSpec(loadSpec()), []);
});

test('a well-formed specification has no problems', () => {
  assert.deepEqual(validateSpec(validSpec()), []);
});

test('reports a missing word category', () => {
  const spec = validSpec();
  delete spec.words.booleans;

  assert.deepEqual(validateSpec(spec), ["words.booleans must be a non-empty list"]);
});

test('reports a word listed in two categories', () => {
  const spec = validSpec();
  spec.words.booleans = ['si'];
  spec.words.controlKeywords = ['si'];

  assert.deepEqual(validateSpec(spec), ["'si' is listed more than once"]);
});

test('reports a word that is not a plain identifier', () => {
  const spec = validSpec();
  spec.words.controlKeywords = ['si|sinon'];

  assert.deepEqual(validateSpec(spec), ["'si|sinon' is not an identifier"]);
});

test('reports a deprecated word whose replacement is not listed', () => {
  const spec = validSpec();
  spec.deprecatedWords = { mostran: 'mostraln' };

  assert.deepEqual(validateSpec(spec), ["deprecated word 'mostran' is replaced by 'mostraln', which is not listed"]);
});

test('reports a deprecated word that is still listed', () => {
  const spec = validSpec();
  spec.deprecatedWords = { w1: 'w0' };

  assert.deepEqual(validateSpec(spec), ["deprecated word 'w1' is still listed"]);
});

test('reports a deprecated word that is not an identifier', () => {
  const spec = validSpec();
  spec.deprecatedWords = { 'si-non': 'w0' };

  assert.deepEqual(validateSpec(spec), ["'si-non' is not an identifier"]);
});

test('reports deprecated words that are not a map of names', () => {
  const spec = validSpec();
  spec.deprecatedWords = ['sinon'];

  assert.deepEqual(validateSpec(spec), ['deprecatedWords must map each old name to its replacement']);
});

test('highlights a deprecated word with the category of its replacement', () => {
  const spec = validSpec();
  spec.deprecatedWords = { mostran: 'w4' };

  assert.deepEqual(highlightedWords(spec, 'builtinFunctions'), ['w4', 'mostran']);
  assert.deepEqual(highlightedWords(spec, 'controlKeywords'), ['w1']);
});

test('the old spelling mostran is deprecated for mostraln', () => {
  const spec = loadSpec();

  assert.deepEqual(spec.deprecatedWords, { mostran: 'mostraln' });
  assert.ok(spec.words.controlKeywords.includes('sinon'));
  assert.ok(spec.words.builtinFunctions.includes('mostraln'));
});

test('tenta is no longer a word of the language, spia replaced it', () => {
  const spec = loadSpec();

  assert.ok(!allWords(spec).includes('tenta'));
  assert.ok(spec.words.controlKeywords.includes('spia'));
});

test('reports missing file extensions', () => {
  const spec = validSpec();
  spec.fileExtensions = [];

  assert.deepEqual(validateSpec(spec), ['fileExtensions must be a non-empty list']);
});

test('the entry function inisiu is a plain name, not a reserved word', () => {
  assert.ok(!allWords(loadSpec()).includes('inisiu'));
});
