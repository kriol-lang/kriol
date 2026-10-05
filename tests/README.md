# Kriol Tests

This directory contains source-level regression tests, small C/C++ API tests,
runtime tests, and fuzzing harnesses for the Kriol compiler.

## Running Tests

Run the normal suite from the repository root:

```sh
cmake -B build
cmake --build build --target check
```

The `check` target runs:

- `tests/api/compile_memory.cc`
- `tests/api/type_model.cc`
- `tests/runtime/gc_stress.c`
- `tests/fuzz/compile_fuzz_smoke.cc`
- `run_tests.sh`

`run_tests.sh` also compiles and runs examples, pass, panic and fail tests,
inline source text and command-line error cases. When the compiler has wasm
support and `node` is installed, it runs the examples, pass and panic tests
again as wasm32-wasi modules through `tests/wasm/run_wasi.mjs`.
`run_tests.ps1` is the Windows counterpart, without the wasm runs.

## Directory Layout

- `pass/`: Kriol programs expected to compile successfully and execute with exit
  code 0.
- `panic/`: Kriol programs expected to compile and then stop with a runtime
  error.
- `fail/`: Kriol programs expected to be rejected by the compiler.
- `api/`: C++ tests for compiler API behavior that is awkward to express as
  source files.
- `runtime/`: C runtime tests, currently focused on garbage-collector stress.
- `fuzz/`: Fuzzing harnesses and deterministic smoke/random fuzz runners.

## Pass Tests

Files in `tests/pass/*.kr` are compiled to a temporary native executable and
executed with a 5 second timeout. A pass test fails if compilation fails, the
program exits non-zero, or it times out.

Two optional fixtures sit next to a test (and next to the examples):

- `<test>.kr.stdin`: the program's standard input.
- `<test>.kr.stdout`: the exact output the program must write; an empty file
  means it must write nothing.

## Panic Tests

Files in `tests/panic/*.kr` must compile and then stop at runtime, such as an
array index out of bounds, a failed `konfirma`, a division by zero or an
unhandled `Erru`. A `// expect: <text>` line names the message the program must
write to stderr, and a `// exit: <code>` line the exact exit status (any
non-zero status otherwise). They use the same `.stdin` and `.stdout` fixtures,
so a `.stdout` file also checks that nothing runs after the error.

Keep pass tests deterministic. Do not depend on external files, network access,
wall-clock time, or platform-specific output unless the test is explicitly about
that behavior.

## Fail Tests

Files in `tests/fail/*.kr` are expected to fail compilation. A
`// expect: <text>` line names a diagnostic the compiler must report.

Use fail tests for parser, semantic-analysis, and codegen rejection regressions.
When a fuzz run finds an internal compiler exception, reduce it to the smallest
source file that reproduces the issue and add that reduced file here after the
fix.

## API And Runtime Tests

`tests/api` and `tests/runtime` are compiled as standalone executables by CMake.
Use them when the behavior under test is not naturally represented by a Kriol
source file, such as in-memory compile API behavior, internal type-model rules,
or runtime implementation details.

## Coverage

Build an instrumented compiler with Clang and run the `coverage` target:

```sh
CC=clang CXX=clang++ cmake -B build-coverage -DKRIOL_ENABLE_COVERAGE=ON
cmake --build build-coverage --target coverage
```

It runs the API tests and `run_tests.sh`, prints a per-file report of the
compiler's sources and writes an HTML report to `build-coverage/coverage/html`.

## Fuzzing

The fuzz harness compiles generated source text to LLVM IR in memory. Expected
syntax and scanner failures are swallowed by the harness; uncaught exceptions,
assertions, sanitizer findings, and process crashes are treated as bugs.

The normal `check` target runs only `kriol_compile_fuzz_smoke`, which feeds a
small fixed corpus through the harness to verify that the fuzz entry point still
works.

Run deterministic random crash hunting with:

```sh
cmake --build build --target random-fuzz
```

Useful knobs:

```sh
KRIOL_RANDOM_FUZZ_SEED=1234
KRIOL_RANDOM_FUZZ_ITERATIONS=10000
KRIOL_RANDOM_FUZZ_MAX_BYTES=4096
```

You can also run the binary directly:

```sh
KRIOL_RANDOM_FUZZ_SEED=1234 KRIOL_RANDOM_FUZZ_ITERATIONS=10000 \
  ./build/kriol_compile_random_fuzz
```

If random fuzzing hits an unexpected compiler exception, it writes
`kriol-random-fuzz-reproducer.kr` in the repository root and prints the seed and
iteration. Keep the seed and iteration in the bug report or commit message so
the generator path remains reproducible.

Run coverage-guided fuzzing with libFuzzer and sanitizers:

```sh
cmake -B build/fuzz -DKRIOL_ENABLE_FUZZING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/fuzz --target kriol_compile_fuzzer
mkdir -p build/fuzz-corpus build/fuzz-artifacts
./build/fuzz/kriol_compile_fuzzer build/fuzz-corpus \
  -artifact_prefix=build/fuzz-artifacts/
```

In restricted debugger/sandbox environments, LeakSanitizer may fail before
reporting useful results. If that happens, rerun with:

```sh
LSAN_OPTIONS=detect_leaks=0 ./build/fuzz/kriol_compile_fuzzer build/fuzz-corpus
```

AddressSanitizer and UndefinedBehaviorSanitizer findings still remain active in
that mode.
