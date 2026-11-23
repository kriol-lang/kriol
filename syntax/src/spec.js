import { readFileSync } from 'node:fs';

export const SPEC_URL = new URL('../kriol-syntax.json', import.meta.url);

export const WORD_CATEGORIES = Object.freeze([
  'declarationKeywords',
  'controlKeywords',
  'builtinTypes',
  'booleans',
  'builtinFunctions',
  'preludeTypes',
]);

const IDENTIFIER = /^[A-Za-z_][A-Za-z0-9_]*$/;

export function loadSpec(url = SPEC_URL) {
  return JSON.parse(readFileSync(url, 'utf8'));
}

export function allWords(spec) {
  return WORD_CATEGORIES.flatMap((category) => spec.words[category] ?? []);
}

// Old spellings that still compile, with a warning, mapped to the word that replaced them.
export function deprecatedWordsOf(spec) {
  return spec.deprecatedWords ?? {};
}

// The words coloured like a category: its own, then the old spellings of them.
export function highlightedWords(spec, category) {
  const words = spec.words[category];
  const oldSpellings = Object.entries(deprecatedWordsOf(spec))
    .filter(([, replacement]) => words.includes(replacement))
    .map(([oldName]) => oldName);
  return [...words, ...oldSpellings];
}

function isNonEmptyList(value) {
  return Array.isArray(value) && value.length > 0;
}

function duplicates(words) {
  return [...new Set(words.filter((word, i) => words.indexOf(word) !== i))];
}

function isNameMap(value) {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
    && Object.values(value).every((name) => typeof name === 'string');
}

function deprecatedWordProblems(deprecatedWords, words) {
  if (!isNameMap(deprecatedWords))
    return ['deprecatedWords must map each old name to its replacement'];

  return Object.entries(deprecatedWords).flatMap(([oldName, replacement]) => {
    if (!IDENTIFIER.test(oldName)) return [`'${oldName}' is not an identifier`];
    if (words.includes(oldName)) return [`deprecated word '${oldName}' is still listed`];
    if (!words.includes(replacement))
      return [`deprecated word '${oldName}' is replaced by '${replacement}', which is not listed`];
    return [];
  });
}

// Every problem that makes the specification unusable; empty when it is valid.
export function validateSpec(spec) {
  const problems = [];

  if (!isNonEmptyList(spec.fileExtensions))
    problems.push('fileExtensions must be a non-empty list');
  for (const category of WORD_CATEGORIES) {
    if (!isNonEmptyList(spec.words?.[category]))
      problems.push(`words.${category} must be a non-empty list`);
  }

  const words = allWords(spec);
  for (const word of words.filter((word) => !IDENTIFIER.test(word)))
    problems.push(`'${word}' is not an identifier`);
  for (const word of duplicates(words))
    problems.push(`'${word}' is listed more than once`);
  problems.push(...deprecatedWordProblems(deprecatedWordsOf(spec), words));

  return problems;
}
