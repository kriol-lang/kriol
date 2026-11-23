import { test } from 'node:test';
import assert from 'node:assert/strict';
import { buildArtifacts } from '../src/build.js';
import { WORD_CATEGORIES, loadSpec } from '../src/spec.js';

const spec = loadSpec();

// dist/ is not committed: the tests import the module as the build would write it.
function importBuiltModule() {
  const source = buildArtifacts(spec)['index.js'];
  return import(`data:text/javascript,${encodeURIComponent(source)}`);
}

test('the module exports every word list of the specification', async () => {
  const kriol = await importBuiltModule();

  for (const category of WORD_CATEGORIES)
    assert.deepEqual(kriol[category], spec.words[category], category);
  assert.deepEqual(kriol.keywords, [...spec.words.declarationKeywords, ...spec.words.controlKeywords]);
  assert.deepEqual(kriol.fileExtensions, spec.fileExtensions);
  assert.deepEqual(kriol.comments, spec.comments);
  assert.deepEqual(kriol.deprecatedWords, spec.deprecatedWords);
});

test('the exported lists cannot be changed by a consumer', async () => {
  const kriol = await importBuiltModule();

  assert.ok(Object.isFrozen(kriol.controlKeywords));
  assert.ok(Object.isFrozen(kriol.comments.block));
  assert.ok(Object.isFrozen(kriol.deprecatedWords));
});

test('the type declarations name every export', () => {
  const declarations = buildArtifacts(spec)['index.d.ts'];

  for (const name of [...WORD_CATEGORIES, 'keywords', 'fileExtensions', 'comments', 'scopeName', 'deprecatedWords'])
    assert.match(declarations, new RegExp(`export declare const ${name}:`), name);
});
