# Changelog

All notable changes to the Kriol compiler are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [1.17.0] - 2026-11-10

### Added

- Add `spia { } oops (err) { } sempri { }`, the Kriol try/catch/finally. The
  `oops` block catches an `Erru` raised with `lansa` inside the `spia`, one
  returned by a call at any depth, a failed `toma()` or `konverti()`, and the
  runtime errors that used to end the program: division by zero, a division
  result that does not fit its type, an index out of bounds, a failed
  `konfirma` and `paniku`. The program continues after the block, `lansa err;`
  inside `oops` raises the error again, and `sempri` runs on every way out of
  the block, including `divolvi`, `kebra` and `kontinua`. `sai(n)` and
  out-of-memory are never caught. Programs without `spia` compile to the same
  code and print the same messages as before. `spia` works in native,
  `x86_64-windows` and `wasm32-wasi` builds. A WebAssembly program that uses
  `spia` needs a host with WebAssembly exception handling; one that does not
  use `spia` is unchanged. ([#32], [e93ac82])
- Give `Erru` three new fields after `mensagem`: `tipu` (`textu`, one of
  `"lansa"`, `"paniku"`, `"divizon"`, `"indisi"`, `"konfirma"`, `"konverti"`
  and `"toma"`), `fixeru` (`textu`) and `linha` (`int`). `Erru{mensagem = "x"}`
  still compiles, and the compiler fills the new fields. ([#32])
- Pass the error of a call on to the caller with `spia f()`. It takes the place
  of `tenta f()`, with the same rules: only in a function declared with
  `: Erru`, in a `spia` body or in an `oops`, and only on a call that can fail.
  ([#35], [#37])
- Accept parentheses around the whole header of a `si` with an initializer and
  of a `pa`, as in `si (int y = f(); y > 0) { }` and
  `pa (int i = 0; i < 10; i += 1) { }`, so every statement header may be
  wrapped in them. Braces are still required. ([e93ac82])
- Accept `oops err { }`, without parentheses around the name. ([e93ac82])
- Ship `CHANGELOG.md` in every release archive, and the zstd license in the
  macOS archive, which links zstd statically.

### Changed

- **Breaking:** remove `tenta`. Write `spia f()` instead; `tenta` is no longer a
  keyword, so it can be used as a name. ([#35], [#37])
- **Breaking:** remove the `f() sinon valor` fallback, also for `toma()` and
  `konverti()`. Handle the error with `spia { } oops (err) { }`; `sinon` is now
  only the `else` of `si`. A leftover `tenta f()` or `f() sinon valor` fails
  with a message that points to `spia`, and the warning for an unhandled call
  now suggests `spia`. ([#35], [#37])
- **Breaking:** the `mensage` field of `Erru` is now `mensagem`, so every
  `Erru{mensage = ...}` and `err.mensage` has to be rewritten. ([#32])
- **Breaking:** `spia`, `oops` and `sempri` are now reserved words, so a
  program that uses one of them as a name no longer compiles. ([#32])
- Rename the built-in `mostran` to `mostraln`. ([#37])
- Pin every GitHub Action to a commit SHA, and the SHA256 of the LLVM and
  llvm-mingw archives the Windows build downloads, since `ld.lld.exe` and the
  MinGW runtime ship in the release. ([#22], [#24])

### Deprecated

- `mostran` still compiles, with a warning that names `mostraln`, and
  `--strict` turns the warning into an error. It stays a reserved name while
  the compiler accepts it. ([#37])

### Fixed

- Reject an exit code outside 0 to 255 in `sai`. A literal one is a compile
  error and a computed one stops the program with an error, on native and wasm
  targets; before, `sai(256)` exited with 0 and reported success. ([#23])
- Apply the local size limit to functions that return a large `molda`, and to
  array and `molda` literals used as temporaries. They crashed with a
  segmentation fault instead of being rejected. ([#30])
- Link macOS programs without a clang warning and one ld warning per object of
  the embedded libgc. ([#34])
- Fail the `check` target when the wasm tests cannot run, instead of silently
  skipping them. ([#25])

### Documentation

- Put the license in the name of the Kriol Lang Project Contributors.
  ([6e20356])
- Add the macOS install instructions to the README. ([f5150c7])

## [1.16.2] - 2026-10-25

### Added

- Build and release a macOS archive for Apple silicon,
  `kriol-<version>-macos-arm64.tar.gz`. It links zstd statically, and CI checks
  that it needs no Homebrew library, so it runs on a Mac without Homebrew. This
  build does not compile to WebAssembly. ([#20], [8a99934])
- Install on macOS with `install.sh`. The script picks the archive for the
  machine, also from a shell running under Rosetta, and stops with a clear
  message on a platform that has no release build. ([c5c1f41])

### Fixed

- Verify the archive `install.sh` downloads against the release
  `SHA256SUMS.txt` before extracting it, so a truncated or swapped download is
  no longer installed silently. ([#21])
- Embed the runtime bitcode with Apple clang. Clang 19, and the Apple clang
  based on it, rejects an `#embed` file whose first byte is above 0x7F, as
  macOS bitcode always is, so the build now probes for this and falls back to a
  generated array. ([8ef3598], [ed0d3e5])
- Use the C and C++ compilers given to CMake instead of replacing them with the
  newest clang found. ([8a99934])
- Run `run_tests.sh` on macOS: find `bash` through `env`, and use `gtimeout`
  from Homebrew coreutils when GNU `timeout` is missing. ([#20], [4e2da1b])

### Documentation

- Add a contributing guide in `CONTRIBUTING.md`, and update the README.

## [1.16.1] - 2026-10-11

### Fixed

- Reject records (`molda`) with recursive fields instead of crashing. A field
  whose type is the record being declared, directly or through an array, now
  gets its own diagnostic, and a field of unknown type is no longer added to the
  record, so computing the storage size can no longer recurse forever on
  self-referential or mutually recursive records. ([#14])
- Reject global initializers that read globals declared later, including through
  a function call or mutual recursion. The compiler reports one initialization
  order error per global. ([#19])
- Prefix generated global variables with `kriol.g.`, so a Kriol global can no
  longer clash with a C library or runtime symbol. ([#17])
- Rename the generated source name string to `__kriol.source_name`, keeping it
  out of the user function namespace. ([#18])
- Compute each record storage size once instead of recomputing it per use, and
  saturate the size rather than overflowing on doubling chains. ([#16])
- Keep the test fixtures from being converted to CRLF on a Windows checkout. A
  new `.gitattributes` marks `.stdout` and `.stdin` fixtures as binary, and
  `run_tests.ps1` normalizes a fixture it reads, so the exact output comparison
  no longer fails on every program that writes a newline. ([4d11ab4])

### Changed

- Fail the Windows CI test step when `run_tests.ps1` fails. ([#15])
- Report why a Windows program test failed: whether it did not compile, timed
  out, or which line of its output differs from the fixture. ([4d11ab4])

### Documentation

- Link the four-minute presentation video in the README.

## [1.16.0] - 2026-10-05

### Added

- Write compiler and runtime messages in Portuguese.

### Changed

- Improve test reporting.

## [1.15.0] - 2026-10-05

### Added

- Check program output, runtime errors and wasm runs in the test suite, and
  collect coverage.

### Fixed

- Fill repeated aggregate array elements with a loop.
- Allow `molda` literals inside f-string interpolations.

## [1.14.0] - 2026-10-04

### Added

- Multi-dimensional arrays, and arrays as parameters and return values.
- Write `molda` literals as `Type{field = value}`.

### Changed

- Rename `para` to `kebra`, and allow `pa` without a step.
- Divide like C, truncate reals into integers, and accept numbers in logical
  operators.
- Update the terminology from `Kuale` to `Kualeh`.

### Fixed

- Pass and return `molda` values by address, like arrays.

Releases before 1.14.0 are not covered here. See the
[git history](https://github.com/kriol-lang/kriol/commits/main) and the
[releases page](https://github.com/kriol-lang/kriol/releases).

[#14]: https://github.com/kriol-lang/kriol/pull/14
[#15]: https://github.com/kriol-lang/kriol/pull/15
[#16]: https://github.com/kriol-lang/kriol/pull/16
[#17]: https://github.com/kriol-lang/kriol/pull/17
[#18]: https://github.com/kriol-lang/kriol/pull/18
[#19]: https://github.com/kriol-lang/kriol/pull/19
[#20]: https://github.com/kriol-lang/kriol/pull/20
[#21]: https://github.com/kriol-lang/kriol/pull/21
[#22]: https://github.com/kriol-lang/kriol/pull/22
[#23]: https://github.com/kriol-lang/kriol/pull/23
[#24]: https://github.com/kriol-lang/kriol/pull/24
[#25]: https://github.com/kriol-lang/kriol/pull/25
[#30]: https://github.com/kriol-lang/kriol/pull/30
[#32]: https://github.com/kriol-lang/kriol/pull/32
[#34]: https://github.com/kriol-lang/kriol/pull/34
[#35]: https://github.com/kriol-lang/kriol/issues/35
[#37]: https://github.com/kriol-lang/kriol/pull/37
[4d11ab4]: https://github.com/kriol-lang/kriol/commit/4d11ab414aeeec30c60071006d34fea38a44f020
[4e2da1b]: https://github.com/kriol-lang/kriol/commit/4e2da1b69890b3dd9f9e65136e5b9bd032e2285a
[6e20356]: https://github.com/kriol-lang/kriol/commit/6e203562174608538d81a7ba02a414e1376bd7b9
[8a99934]: https://github.com/kriol-lang/kriol/commit/8a99934e7781e35ccbe8e8f24c9593d1f4695f54
[8ef3598]: https://github.com/kriol-lang/kriol/commit/8ef35984c1d0bbacb2ce281083b3fd85e1309778
[c5c1f41]: https://github.com/kriol-lang/kriol/commit/c5c1f4197c4c8e2ce9f53806ee134878cd1aa402
[e93ac82]: https://github.com/kriol-lang/kriol/commit/e93ac823a1739876b7e58e14c1e55246fd687e48
[ed0d3e5]: https://github.com/kriol-lang/kriol/commit/ed0d3e5ffc2f83f67db58ef326f10c8a5cee89d1
[f5150c7]: https://github.com/kriol-lang/kriol/commit/f5150c786cc7ea04f9a8d0bc786690013ac86b2e
[1.17.0]: https://github.com/kriol-lang/kriol/compare/v1.16.2-alpha+build1...v1.17.0-alpha+build1
[1.16.2]: https://github.com/kriol-lang/kriol/compare/v1.16.1-alpha+build2...v1.16.2-alpha+build1
[1.16.1]: https://github.com/kriol-lang/kriol/compare/v1.16.0-alpha+build2...v1.16.1-alpha+build2
[1.16.0]: https://github.com/kriol-lang/kriol/compare/v1.15.0-alpha+build1...v1.16.0-alpha+build2
[1.15.0]: https://github.com/kriol-lang/kriol/compare/v1.14.0-alpha+build1...v1.15.0-alpha+build1
[1.14.0]: https://github.com/kriol-lang/kriol/compare/v1.13.1-alpha+build1...v1.14.0-alpha+build1
