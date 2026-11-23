import { test } from 'node:test';
import assert from 'node:assert/strict';
import { releaseVersion } from '../src/release.js';

test('the package version is the release tag without its v', () => {
  assert.equal(releaseVersion('v1.17.0-alpha+build1', 'Kriol v1.17.0\n'), '1.17.0-alpha+build1');
  assert.equal(releaseVersion('v1.17.0', 'Kriol v1.17.0\n'), '1.17.0');
});

test('refuses a tag for another version than the compiler', () => {
  assert.throws(
    () => releaseVersion('v1.18.0-alpha+build1', 'Kriol v1.17.0\n'),
    /v1\.18\.0-alpha\+build1 does not match the compiler's version 1\.17\.0/,
  );
});

test('refuses a tag that is not a release tag', () => {
  assert.throws(() => releaseVersion('main', 'Kriol v1.17.0\n'), /'main' is not a release tag/);
  assert.throws(() => releaseVersion('v1.17', 'Kriol v1.17.0\n'), /'v1\.17' is not a release tag/);
});

test('refuses output that kriol --version does not print', () => {
  assert.throws(() => releaseVersion('v1.17.0', 'kriol: command not found'), /not the output of kriol --version/);
});
