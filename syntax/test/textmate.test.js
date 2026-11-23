import { test } from 'node:test';
import assert from 'node:assert/strict';
import { WORD_CATEGORIES, loadSpec } from '../src/spec.js';
import { WORD_SCOPES, buildTextMateGrammar } from '../src/textmate.js';

const spec = loadSpec();
const grammar = buildTextMateGrammar(spec);

// The patterns of a repository entry that carry a `match` regex, by scope name.
// The regexes only use syntax that Oniguruma and JavaScript read the same way.
function matchPatterns(entry) {
  return new Map(
    grammar.repository[entry].patterns
      .filter((pattern) => pattern.match)
      .map((pattern) => [pattern.name, new RegExp(`^(?:${pattern.match})$`)]),
  );
}

function allMatchPatterns() {
  return new Map(Object.keys(grammar.repository).flatMap((entry) =>
    grammar.repository[entry].patterns ? [...matchPatterns(entry)] : []));
}

test('identifies the language by the compiler extensions', () => {
  assert.equal(grammar.scopeName, 'source.kriol');
  assert.equal(grammar.name, 'kriol');
  assert.deepEqual(grammar.fileTypes, ['kriol', 'kr']);
  assert.deepEqual(grammar.aliases, ['kr']);
});

for (const [category, scope] of Object.entries(WORD_SCOPES)) {
  test(`colours every word of ${category} as ${scope}`, () => {
    const pattern = allMatchPatterns().get(scope);
    assert.ok(pattern, `no pattern for ${scope}`);

    for (const word of spec.words[category]) {
      assert.match(word, pattern);
      assert.doesNotMatch(`${word}x`, pattern);
      assert.doesNotMatch(`x${word}`, pattern);
    }
  });
}

test('colours a deprecated word like the word that replaces it', () => {
  for (const [oldName, replacement] of Object.entries(spec.deprecatedWords)) {
    const category = WORD_CATEGORIES.find((name) => spec.words[name].includes(replacement));
    assert.match(oldName, allMatchPatterns().get(WORD_SCOPES[category]), oldName);
  }
});

test('does not colour the entry function inisiu as a keyword', () => {
  for (const [scope, pattern] of allMatchPatterns()) {
    if (scope.startsWith('keyword.'))
      assert.doesNotMatch('inisiu', pattern, scope);
  }
});

test('prelude types win over the generic type name rule', () => {
  const scopes = grammar.repository.types.patterns.map((pattern) => pattern.name);

  assert.ok(scopes.indexOf('support.type.kriol') < scopes.indexOf('entity.name.type.kriol'));
});

test('colours comments with the delimiters of the specification', () => {
  const comments = grammar.repository.comments.patterns;
  const line = new RegExp(comments.find((pattern) => pattern.match).match);
  const block = comments.find((pattern) => pattern.begin);

  assert.match('// oi', line);
  assert.equal(block.begin, '/\\*');
  assert.equal(block.end, '\\*/');
});

test('f-string interpolations allow nested braces, as in a molda literal', () => {
  for (const string of grammar.repository.strings.patterns) {
    const interpolation = string.patterns.find((pattern) => pattern.name === 'meta.interpolation.kriol');

    assert.deepEqual(interpolation.patterns, [{ include: '#braces' }, { include: '$self' }]);
  }
});
