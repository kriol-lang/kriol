// Turns the specification into the files consumers import from dist/.

import { WORD_CATEGORIES, deprecatedWordsOf } from './spec.js';
import { buildTextMateGrammar } from './textmate.js';

export const DIST_DIR = new URL('../dist/', import.meta.url);

const HEADER = '// Generated from kriol-syntax.json by `npm run build`. Do not edit it by hand.';

function exportedLists(spec) {
  return {
    fileExtensions: spec.fileExtensions,
    ...Object.fromEntries(WORD_CATEGORIES.map((category) => [category, spec.words[category]])),
    keywords: [...spec.words.declarationKeywords, ...spec.words.controlKeywords],
  };
}

function frozenList(list) {
  return `Object.freeze(${JSON.stringify(list)})`;
}

function tupleType(list) {
  return `readonly [${list.map((item) => JSON.stringify(item)).join(', ')}]`;
}

function recordType(record) {
  const fields = Object.entries(record).map(([key, value]) => `readonly ${key}: ${JSON.stringify(value)}`);
  return `{ ${fields.join('; ')} }`;
}

function buildModule(spec) {
  const { line, block } = spec.comments;
  const lines = [
    HEADER,
    '',
    `export const name = ${JSON.stringify(spec.name)};`,
    `export const scopeName = ${JSON.stringify(spec.scopeName)};`,
    `export const comments = Object.freeze({ line: ${JSON.stringify(line)}, block: ${frozenList(block)} });`,
    ...Object.entries(exportedLists(spec)).map(([name, list]) => `export const ${name} = ${frozenList(list)};`),
    '// Old spellings that still compile, with a warning, mapped to their replacement.',
    `export const deprecatedWords = Object.freeze(${JSON.stringify(deprecatedWordsOf(spec))});`,
  ];
  return `${lines.join('\n')}\n`;
}

function buildTypes(spec) {
  const { line, block } = spec.comments;
  const lines = [
    HEADER,
    '',
    `export declare const name: ${JSON.stringify(spec.name)};`,
    `export declare const scopeName: ${JSON.stringify(spec.scopeName)};`,
    `export declare const comments: { readonly line: ${JSON.stringify(line)}; readonly block: ${tupleType(block)} };`,
    ...Object.entries(exportedLists(spec)).map(([name, list]) => `export declare const ${name}: ${tupleType(list)};`),
    `export declare const deprecatedWords: ${recordType(deprecatedWordsOf(spec))};`,
  ];
  return `${lines.join('\n')}\n`;
}

// File name inside dist/ mapped to its content.
export function buildArtifacts(spec) {
  return {
    'kriol.tmLanguage.json': `${JSON.stringify(buildTextMateGrammar(spec), null, 2)}\n`,
    'index.js': buildModule(spec),
    'index.d.ts': buildTypes(spec),
  };
}
