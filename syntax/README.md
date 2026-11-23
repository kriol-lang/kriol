# kriol-syntax

The single source of the KriolLang syntax for editors and sites. It holds the
words of the language (keywords, types, booleans and prelude functions) in
[`kriol-syntax.json`](kriol-syntax.json) and generates from them:

| File | What it is | Used by |
|---|---|---|
| `dist/kriol.tmLanguage.json` | TextMate grammar | VS Code, the docs (Starlight), the website (Shiki), GitHub |
| `dist/index.js` + `index.d.ts` | the word lists as frozen arrays | the playground (CodeMirror), the website |

It lives in the compiler's repository so that the two change together. The
compiler is the source of truth: the hidden option `kriol --print-words` prints
the words from the tables the compiler itself uses (the scanner's in
`rules/scanner.l`, the prelude's in `include/kriol/prelude.hh` and the renamed
words in `include/kriol/deprecated.hh`), and a check fails when they and
`kriol-syntax.json` disagree. It runs with the compiler's own tests, so a pull
request that adds a keyword fails until this file follows it.

The tree-sitter grammar ([kriol-tree-sitter](https://github.com/kriol-lang/kriol-tree-sitter))
stays a separate parser; its word lists should follow this package.

## Usage

Every compiler release attaches the package, with the same version as the
compiler, and the TextMate grammar on its own:

```sh
npm install https://github.com/kriol-lang/kriol/releases/download/<tag>/kriol-syntax-<version>.tgz
```

where `<version>` is the tag without its `v`: release `v1.17.0-alpha+build1`
ships `kriol-syntax-1.17.0-alpha+build1.tgz`.

```js
import { controlKeywords, builtinTypes, comments } from 'kriol-syntax';
import grammar from 'kriol-syntax/kriol.tmLanguage.json' with { type: 'json' };
```

| Export | Content |
|---|---|
| `name`, `scopeName` | `kriol`, `source.kriol` |
| `fileExtensions` | `kriol`, `kr` |
| `comments` | `{ line: '//', block: ['/*', '*/'] }` |
| `declarationKeywords` | `fn`, `molda`, `inpristan` |
| `controlKeywords` | `si`, `sinon`, `pa`, `nkuantu`, `kebra`, `kontinua`, `divolvi`, `lansa`, `spia`, `oops`, `sempri` |
| `keywords` | both lists above |
| `builtinTypes` | `num`, `int`, `bool`, `textu`, `i8` … `u64`, `f32`, `f64`, `isize`, `usize` |
| `booleans` | `sin`, `nau` |
| `builtinFunctions` | `mostra`, `mostraln`, `toma`, `sai`, `konfirma`, `paniku` |
| `preludeTypes` | `Erru` |
| `deprecatedWords` | `{ mostran: 'mostraln' }` |

`deprecatedWords` maps an old spelling to the word that replaced it. The
compiler still accepts the old spelling and warns about it. The TextMate
grammar colours it like its replacement. The word lists leave it out, so an
editor that completes words from them only suggests the current spelling.

## Changing the language

When the compiler gains or loses a word:

1. Edit `kriol-syntax.json` in the same pull request.
   To rename a word, put the new name in its list and map the old one to it
   in `deprecatedWords` while the compiler still accepts it.
2. Run the compiler's whole suite with `cmake --build build --target check`
   from the repository root. It runs this package's tests and compares the
   words with the compiler it just built.

On its own, `KRIOL=../build/kriol npm test` runs the tests here, and
`npm run check:compiler -- ../build/kriol` only the comparison with a compiler.
Without `KRIOL`, `npm test` skips the comparison.
`npm run build` writes `dist/` for local use; it is not committed.

## Releases

The release workflow packs the package from the compiler's tag with
`node scripts/pack.js <tag> <path/to/kriol> <out-dir>`. It refuses a tag whose
version is not the one `kriol --version` prints, so the package always carries
the version of the compiler it was checked against. That version is set once,
in `project()` in the repository's `CMakeLists.txt`.

Requires Node 22 or later and has no dependencies.

## License

[MIT](https://github.com/kriol-lang/kriol/blob/main/LICENSE)
