# Contributing to Kriol

Thanks for your interest in Kriol. The language is young, so bug reports, test
cases and documentation fixes are as welcome as compiler changes.

This guide covers how to get set up, what a good change looks like, and how it
gets reviewed. If something here is unclear or wrong, that is a bug in this file,
so please open an issue.

## Ways to contribute

- **Report a bug.** A program that crashes the compiler, is wrongly rejected, or
  produces the wrong result is the most useful report there is. Include the
  smallest source file that reproduces it, the command you ran, what you
  expected, and what happened, along with your platform and `kriol --version`.
- **Fix a bug.** Small, focused pull requests get reviewed fastest.
- **Add test cases.** Coverage of the existing language is still thin. A pull
  request that only adds tests is a real contribution.
- **Improve the documentation.** The language reference lives in a separate
  repository and is published at [docs.kriol.dev](https://docs.kriol.dev).

For anything large, such as new syntax, a new type or a change to an existing
language rule, open an issue first so the design can be discussed before you
spend time on it.

## Getting set up

Building the compiler currently works mostly on Linux. It needs **Clang/LLVM 20**
and **LLD 20** to build and to link user programs, plus **bison** and **flex**
for the language grammar. On Debian or Ubuntu:

```sh
apt install make flex bison clang-20 llvm-20 llvm-20-dev llvm-20-tools lld-20 \
    zlib1g-dev libzstd-dev cmake
```

WASI output, which is on by default, also needs:

```sh
apt install wasi-libc libclang-rt-20-dev-wasm32
```

Then build and run it:

```sh
cmake -B build && cmake --build build
./build/kriol --help
```

The [README](README.md) has the full build options, including release builds, a
native-only build with `-DKRIOL_ENABLE_WASM=OFF`, and how to build on Windows.

`kriol --text '<source>'` compiles a snippet without creating a file, which is
the quickest way to check compiler behaviour while you work:

```sh
./build/kriol --text 'fn inisiu() { mostraln("Kualeh, Mundu!"); }' -o hello && ./hello
```

## Running the tests

Run the suite from the repository root before you open a pull request:

```sh
cmake -B build && cmake --build build --target check
```

On Windows the counterpart is:

```powershell
.\run_tests.ps1 build\kriol.exe .
```

[`tests/README.md`](tests/README.md) is the reference for the test suite. It
explains the directory layout, the fixtures, coverage and the fuzzing harnesses
in detail. In short:

| Directory | What the program must do |
| --- | --- |
| `tests/pass/` | compile, run and exit 0 |
| `tests/panic/` | compile, then stop with a runtime error |
| `tests/fail/` | be rejected by the compiler |
| `tests/api/`, `tests/runtime/` | C++ and C tests for behaviour that no Kriol source file expresses naturally |

A `<test>.kr.stdout` fixture is the exact output the program must write, and a
`<test>.kr.stdin` fixture is its standard input. In `tests/fail` and
`tests/panic`, a `// expect: <text>` line names the message that must be
reported, and in `tests/panic` a `// exit: <code>` line names the exact exit
status.

## What needs a test

Nearly every change does. A fix should come with the smallest source file that
failed before it and passes after it, added under the directory that matches the
behaviour. Keep tests deterministic: no network, no wall-clock time, no
dependence on files outside the repository.

When a change genuinely cannot be tested here, say so in the commit body and why,
on a line starting with `No test:`, and say how you verified it instead. CI
workflow changes are the usual case.

## Writing the code

The compiler is C++ under `src/kriol`, with its headers in `include/kriol`, and
the C runtime is under `runtime`. There is no `.clang-format` in the repository,
so match the file you are editing. Across the existing sources that means four
space indentation, braces on the same line as the statement that opens them,
`lowerCamelCase` functions, and types taken by `const&` rather than by value.

Comments explain *why* the code does something, not what it does. The existing
ones are worth reading as a model: they tend to record the constraint that forced
the design, which is what a future reader cannot recover from the code.

Keep a change to one concern. A fix that also renames things and reformats a file
is slow to review and hard to revert.

### Compiler and runtime messages are in Portuguese

Every diagnostic the compiler and the runtime write is in Portuguese, so a new
error or warning message must be written in Portuguese too, matching the
phrasing of the messages around it. The `// expect:` lines in `tests/fail` and
`tests/panic` are the easiest place to see the existing wording.

## Commit messages

The history follows [Conventional Commits](https://www.conventionalcommits.org),
with the subject in the imperative mood and capitalized after the prefix:

```
fix: Reject recursive molda fields instead of crashing
```

The prefixes in use are `feat`, `fix`, `refactor`, `docs`, `test`, `style`,
`build` and `chore`. Keep the subject under about 72 characters and without a
trailing period. Use the body, wrapped at about 76 characters, to say what the
change does and why it was needed, which matters far more than restating the
diff.

## Pull requests

1. Fork the repository and branch off `main`.
2. Make the change, add the test, and run `check` locally.
3. Open the pull request against `main` and describe what it fixes and how you
   verified it.

Opening a pull request runs [the CI workflow](.github/workflows/ci.yml), which
builds the compiler on Linux and runs the `check` target. The
[Linux](.github/workflows/linux.yml) and
[Windows](.github/workflows/windows.yml) build workflows are not part of that
run: they fire on a release tag, or on demand from the Actions tab. So if your
change could behave differently on Windows, say so in the pull request, and a
maintainer can run the Windows workflow against it. When it fails,
`run_tests.ps1` reports why each program test failed underneath its `FAIL` line,
so read that rather than only the list of names at the end.

A reviewer will be looking for whether the change is correct at the right layer
of the compiler, whether it has a test that would have caught the bug, and
whether the commit message explains why it was needed.

## The changelog

[`CHANGELOG.md`](CHANGELOG.md) follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/). You do not have to
update it in your pull request, and it is fine if you do. Either way, the entry
is written when a release is cut, under **Added**, **Changed**, **Deprecated**,
**Fixed** or **Documentation**, and references the pull request that made the
change.

## Credit

Once your pull request is merged you appear in the contributor grid in the
[README](README.md#contributors), which is generated from the commit history, so
nobody has to remember to add you.

Release notes credit contributors by hand. Whoever cuts a release can list who
contributed since the previous tag with:

```sh
git shortlog -sn --no-merges <previous-tag>..HEAD
```

A revision is required: without one, `git shortlog` reads from standard input
and prints nothing. If you contributed and the release notes missed you, that is
an oversight and not a judgement, so please say so and it will be fixed.

## License

Kriol is released under the [MIT License](LICENSE). By contributing you agree
that your contribution is licensed under the same terms.
