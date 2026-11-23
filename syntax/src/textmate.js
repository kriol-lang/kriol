// Builds the TextMate grammar used by VS Code, Shiki (the docs and the website)
// and GitHub. Only the word lists come from the specification; the rest
// mirrors the scanner of the compiler (rules/scanner.l).

import { highlightedWords } from './spec.js';

export const WORD_SCOPES = Object.freeze({
  declarationKeywords: 'keyword.other.kriol',
  controlKeywords: 'keyword.control.kriol',
  builtinFunctions: 'support.function.kriol',
  builtinTypes: 'storage.type.kriol',
  preludeTypes: 'support.type.kriol',
  booleans: 'constant.language.boolean.kriol',
});

// The specification only allows identifiers, so the words need no escaping.
// Old spellings still compile, so they are coloured like their replacement.
function wordPattern(spec, category) {
  return {
    name: WORD_SCOPES[category],
    match: `\\b(${highlightedWords(spec, category).join('|')})\\b`,
  };
}

function escapeRegex(text) {
  return text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

// Interpolations may hold braces, such as a molda literal, and other strings.
const INTERPOLATION = {
  name: 'meta.interpolation.kriol',
  begin: '\\{',
  end: '\\}',
  patterns: [{ include: '#braces' }, { include: '$self' }],
};

function stringPattern(quote, kind) {
  return {
    name: `string.quoted.${kind}.kriol`,
    begin: `f?${quote}`,
    end: quote,
    patterns: [
      { name: 'constant.character.escape.kriol', match: '\\\\.' },
      INTERPOLATION,
    ],
  };
}

function commentPatterns({ line, block: [open, close] }) {
  return [
    { name: 'comment.line.double-slash.kriol', match: `${escapeRegex(line)}.*$` },
    { name: 'comment.block.kriol', begin: escapeRegex(open), end: escapeRegex(close) },
  ];
}

export function buildTextMateGrammar(spec) {
  return {
    information_for_contributors: [
      'Generated from kriol-syntax.json by `npm run build` in kriol/syntax. Do not edit it by hand.',
    ],
    name: spec.name,
    displayName: spec.displayName,
    aliases: spec.fileExtensions.filter((extension) => extension !== spec.name),
    scopeName: spec.scopeName,
    fileTypes: spec.fileExtensions,
    patterns: [
      { include: '#comments' },
      { include: '#strings' },
      { include: '#keywords' },
      { include: '#types' },
      { include: '#literals' },
      { include: '#identifiers' },
      { include: '#operators' },
    ],
    repository: {
      braces: {
        comment: 'Braces inside an interpolation, such as a molda literal, so its } does not end the interpolation.',
        begin: '\\{',
        end: '\\}',
        patterns: [{ include: '#braces' }, { include: '$self' }],
      },
      comments: { patterns: commentPatterns(spec.comments) },
      strings: { patterns: [stringPattern('"', 'double'), stringPattern("'", 'single')] },
      keywords: {
        patterns: [
          wordPattern(spec, 'controlKeywords'),
          wordPattern(spec, 'declarationKeywords'),
          wordPattern(spec, 'builtinFunctions'),
        ],
      },
      types: {
        patterns: [
          wordPattern(spec, 'builtinTypes'),
          wordPattern(spec, 'preludeTypes'),
          { name: 'entity.name.type.kriol', match: '\\b[A-Z][A-Za-z0-9_]*\\b' },
          wordPattern(spec, 'booleans'),
        ],
      },
      literals: {
        patterns: [
          { name: 'constant.numeric.float.kriol', match: '(?<![A-Za-z0-9_])[+-]?[0-9]+\\.[0-9]+\\b' },
          { name: 'constant.numeric.integer.kriol', match: '(?<![A-Za-z0-9_])[+-]?[0-9]+\\b' },
        ],
      },
      identifiers: {
        patterns: [
          { name: 'entity.name.function.kriol', match: '\\b([a-zA-Z_][a-zA-Z0-9_]*)(?=\\s*\\()' },
        ],
      },
      operators: {
        patterns: [
          {
            name: 'keyword.operator.kriol',
            match: '::|\\+\\=|\\-\\=|\\*\\=|\\/\\=|%=|&=|\\|=|\\^=|==|!=|<=|>=|&&|\\|\\||[+\\-*/%<>=!:&|^~]',
          },
        ],
      },
    },
  };
}
