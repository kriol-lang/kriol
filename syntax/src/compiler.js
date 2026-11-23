// Reads the words the compiler actually knows from the compiler itself, so the
// specification can be checked against them. The hidden option --print-words
// prints the tables its scanner and prelude use. The compiler is the source of
// truth: when they differ, the specification is the one to fix.

import { execFileSync } from 'node:child_process';
import { deprecatedWordsOf, highlightedWords } from './spec.js';

// The words of a built compiler: keywords, builtinTypes, booleans,
// builtinFunctions, preludeTypes and deprecatedWords (old name -> current name).
export function readCompilerWords(kriol) {
  return JSON.parse(execFileSync(kriol, ['--print-words'], { encoding: 'utf8' }));
}

function differences(label, inSpec, inCompiler) {
  const spec = new Set(inSpec);
  const compiler = new Set(inCompiler);

  return [
    ...inCompiler.filter((word) => !spec.has(word))
      .map((word) => `${label}: '${word}' is in the compiler but not in the specification`),
    ...inSpec.filter((word) => !compiler.has(word))
      .map((word) => `${label}: '${word}' is in the specification but not in the compiler`),
  ];
}

const renamings = (deprecatedWords) =>
  Object.entries(deprecatedWords ?? {}).map(([oldName, currentName]) => `${oldName} -> ${currentName}`);

// Every difference between the specification and the compiler; empty when they agree.
// The scanner still reads the old spelling of a renamed keyword, so it counts as a keyword.
export function compareWithCompiler(spec, compiler) {
  const { builtinTypes, booleans, builtinFunctions, preludeTypes } = spec.words;
  const keywords = [
    ...highlightedWords(spec, 'declarationKeywords'),
    ...highlightedWords(spec, 'controlKeywords'),
  ];

  return [
    ...differences('keywords', keywords, compiler.keywords),
    ...differences('builtinTypes', builtinTypes, compiler.builtinTypes),
    ...differences('booleans', booleans, compiler.booleans),
    ...differences('builtinFunctions', builtinFunctions, compiler.builtinFunctions),
    ...differences('preludeTypes', preludeTypes, compiler.preludeTypes),
    ...differences('deprecatedWords', renamings(deprecatedWordsOf(spec)), renamings(compiler.deprecatedWords)),
  ];
}
