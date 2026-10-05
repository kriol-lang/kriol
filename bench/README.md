# Benchmarks

Small programs written in Kriol, C and Python, each printing one number:

| Benchmark    | Exercises                                                   |
|--------------|-------------------------------------------------------------|
| `fib`        | Recursive calls (`fibo(35)`, about 30 million calls)        |
| `crivo`      | Array access: sieve of Eratosthenes up to 10⁶, ten times    |
| `matriz`     | Nested loops over reals: product of two 400 × 400 matrices  |
| `mandelbrot` | Real arithmetic: Mandelbrot set on an 800 × 600 grid        |

Run them with a Release build of the compiler:

```sh
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target kriol
bench/run.py --kriol build-release/kriol
```

The script checks that every version prints the same number and reports the
median wall time of five runs (`--runs`), the compile times and the program
sizes. C is compiled with `clang -O2` (`--cc`) and Kriol with its default
optimization level, also `-O2`. When `node` is installed, the Kriol programs
also run as wasm32-wasi modules (`--no-wasm` skips them).
