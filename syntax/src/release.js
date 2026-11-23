// The version a release of the package carries: the compiler's release tag.
// The tag must name the version the compiler was built with, so the word lists
// and the grammar always ship with the compiler they were checked against.

const RELEASE_TAG = /^v((\d+\.\d+\.\d+)(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?)$/;
const COMPILER_VERSION = /^Kriol v(\d+\.\d+\.\d+)$/;

// releaseVersion('v1.17.0-alpha+build1', 'Kriol v1.17.0') is '1.17.0-alpha+build1'.
export function releaseVersion(tag, versionOutput) {
  const release = tag.match(RELEASE_TAG);
  if (!release) throw new Error(`'${tag}' is not a release tag such as v1.17.0 or v1.17.0-alpha+build1`);

  const compiler = versionOutput.trim().match(COMPILER_VERSION);
  if (!compiler) throw new Error(`'${versionOutput.trim()}' is not the output of kriol --version`);

  const [, packageVersion, tagVersion] = release;
  const [, compilerVersion] = compiler;
  if (tagVersion !== compilerVersion)
    throw new Error(`the tag ${tag} does not match the compiler's version ${compilerVersion}`);
  return packageVersion;
}
