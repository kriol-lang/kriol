# 🦈 KriolLang

[![Made in Cabo Verde](https://img.shields.io/badge/made%20in-caboverde-008751.svg?style=flat-square)](https://github.com/brocy/made-in-caboverde)

**KriolLang** or **Kriol** is a programming language based on **Cape Verdean Creole**. It was made to be easy for anyone who understands Cape Verdean Creole, while keeping the versatility of the languages its syntax draws on: C, Go and Rust, with Liquid playing a part in the initial design too.

```kriol
fn ola(textu nomi, bool na_kriolu) {
  si na_kriolu {
    mostraln(f"👋 Olá {nomi}, ami nta programa na Kriol!");
  } sinon {
    mostraln(f"👋 Hello {nomi}, I program in Kriol!");
  }
}

fn inisiu() {
  // toma valor di entrada
  textu nomi = toma('Nomi: ');

  // executa funson `ola`
  ola(nomi, sin);
  ola(nomi, nau);
}
```

Source files use the `.kriol` or `.kr` extension, and the compiler is called
`kriol`. The language reference is published at
[docs.kriol.dev](https://docs.kriol.dev).

## Presentation video

A four-minute video that presents the language:

<a href="https://www.youtube.com/watch?v=dkrlfSyyHy0">
  <img src="https://img.youtube.com/vi/dkrlfSyyHy0/maxresdefault.jpg" alt="Kriol: Linguagem di Programason Kabuverdianu, on YouTube" width="640">
</a>

## Install

### Linux

As a dependency, make sure to have a cc linker available like `gcc` or `clang` to ensure proper linkage of the produced object file `.o` into the binary code required to run in the system.

You can install it using the following command:

```sh
curl -fsSL https://raw.githubusercontent.com/kriol-lang/kriol/refs/heads/main/install.sh | sh -s -- --pre
```

After that add the following to  your shell env:

```sh
export PATH="$HOME/.kriol:$PATH"
```

Then close and reopen the shell and try `kriol --version`.

### macOS

Kriol runs on Macs with Apple silicon (M1 or newer). Intel Macs are not
supported yet.

Kriol links programs with `clang`, which comes with the Xcode Command Line
Tools. If you don't have them yet, install them with:

```sh
xcode-select --install
```

Then install Kriol with the same command as on Linux:

```sh
curl -fsSL https://raw.githubusercontent.com/kriol-lang/kriol/refs/heads/main/install.sh | sh -s -- --pre
```

After that add the following to your shell env (`~/.zshrc` is the default on macOS):

```sh
export PATH="$HOME/.kriol:$PATH"
```

Then close and reopen the terminal and try `kriol --version`.

The macOS build compiles native programs only; `--target wasm32-wasi` is not
available on it yet.

### Windows (experimental)

> [!WARNING]
> Windows support is experimental. The Windows binaries are not code-signed
> yet, so Windows may refuse to run them. See
> [If Windows blocks kriol.exe](#if-windows-blocks-kriolexe).

Windows releases need nothing else installed: the zip contains `kriol.exe` and
the `ld.lld.exe` it links programs with, and the programs it builds run on
Windows 7 and later. Install it from PowerShell with:

```powershell
& ([scriptblock]::Create((irm https://raw.githubusercontent.com/kriol-lang/kriol/refs/heads/main/install.ps1))) -Pre
```

This installs to `%LOCALAPPDATA%\Programs\kriol` and adds it to your user
`PATH`; open a new terminal and try `kriol --version`. It needs Windows
PowerShell 5.1 (included in Windows 10 and later, and installable on Windows 7
through WMF 5.1) or PowerShell 7. You can also download the
`kriol-<version>-windows-x86_64.zip` from the releases page and keep its files
together in one folder.

#### If Windows blocks kriol.exe

Because `kriol.exe` and `ld.lld.exe` are not signed, running them can fail
with an error such as *"An Application Control policy has blocked this file"*
(or *"Uma política de Controlo de Aplicações bloqueou este ficheiro"*), or with
a SmartScreen *"Windows protected your PC"* prompt. This comes from Windows
security features, not from kriol.

<details>
<summary><b>How to work around it</b></summary>

1. If you downloaded the zip by hand, unblock the files, then try again. Run
   this in the folder you extracted, or on the zip before extracting it:

   ```powershell
   Get-ChildItem -Recurse | Unblock-File
   ```

   For the SmartScreen prompt, you can also choose **More info → Run anyway**.

2. If the *Application Control policy* error persists, **Smart App Control**
   is blocking unsigned programs. You can check its state in *Windows Security
   → App & browser control → Smart App Control*. In that case we recommend
   using the Linux release inside
   [WSL](https://learn.microsoft.com/windows/wsl/install), which keeps Smart
   App Control on. Install WSL from PowerShell and restart when asked:

   ```powershell
   wsl --install
   ```

   Then, in the WSL (Ubuntu) terminal, install a C compiler and kriol as in the
   [Linux instructions](#linux):

   ```sh
   sudo apt update && sudo apt install -y gcc curl
   curl -fsSL https://raw.githubusercontent.com/kriol-lang/kriol/refs/heads/main/install.sh | sh -s -- --pre
   ```

3. Otherwise, the only way around Smart App Control for now is to turn it off
   in the same place. On many Windows 11 versions it cannot be turned back on
   without resetting Windows, so decide carefully. You can also
   [build kriol from source](#build).

4. On company-managed PCs, the block can come from an IT policy (App Control
   for Business / WDAC). Ask your administrator to allow the files.

</details>

## Usage

`kriol --help` lists every option. To compile one of the files in `examples`:

```sh
kriol examples/fibonacci_recursive.kriol
```

That produces a native binary named `a.out`. Use `-o` to name it yourself:

```sh
kriol examples/fibonacci_recursive.kriol -o fibonacci_recursive
```

Then execute the compiled binary with:

```sh
./fibonacci_recursive
```

Programs are optimized at level 2 by default. Use `--opt-lvl 0` to turn
optimization off (for example, when debugging the compiler), or `--opt-lvl 1`
or `--opt-lvl 3` for the other levels.

The compiler prints warnings (for example, when the error of a function that can
fail is ignored) without stopping. Use `--strict` to treat every warning as an
error:

```sh
kriol examples/ola.kriol --strict
```

To inspect the generated LLVM IR instead of producing a binary (the IR is shown
before optimization):

```sh
kriol examples/mensage_special.kriol --emit-ir
```

Source text can also be compiled directly without creating a source file:

```sh
kriol --text 'fn inisiu() { mostraln("Kualeh, Mundu!"); }' -o hello
```

To compile a Kriol program to a WASI WebAssembly module:

```sh
kriol examples/hello_world.kriol --target wasm32-wasi -o hello.wasm
```

The generated module is a WASI command module. For a browser playground, run it
through a WASI preview1 JavaScript shim and capture stdout to display
`mostra`/`mostraln` output.

Native formatted strings use the embedded Boehm GC runtime. The default WASI
configuration currently uses `malloc` instead: formatted strings are not
reclaimed before process exit, so no-GC WASI output is intended for short-lived
command modules. `KRIOL_WASI_ENABLE_GC=ON` remains experimental and requires a
Boehm GC checkout with WASI support.

## Build

If you want to build the project, currently it only works mostly on Linux based operating systems. It depends mainly on **Clang/LLVM 20** and **LLD 20** to compile the source code and link user programs, and **bison** and **flex** to compile the language rules.

If you are on a Debian (or Ubuntu) based Linux operating system you can install the dependencies using the following command:

```sh
apt install make flex bison clang-20 llvm-20 llvm-20-dev llvm-20-tools lld-20 zlib1g-dev libzstd-dev cmake
```

WASI output also needs the WASI libc and compiler runtime packages:

```sh
apt install wasi-libc libclang-rt-20-dev-wasm32
```

WASM support is enabled by default when building `kriol`. If you want a
native-only compiler with fewer build dependencies, configure with:

```sh
cmake -B build -DKRIOL_ENABLE_WASM=OFF
```

After installing the dependencies, compile the compiler using:

```sh
cmake -B build && cmake --build build
```

And then run with:

```sh
./build/kriol --help
```

For a release build use:

```sh
cmake -B build/release -DCMAKE_BUILD_TYPE=Release -DKRIOL_STATIC=ON && cmake --build build/release
```

And run with:

```sh
./build/release/kriol --help
```

To install it, with its documentation and the licenses of the libraries it
embeds, into a prefix of your choice:

```sh
cmake --install build/release --prefix ~/.local
```

The release archives use a flat layout instead (configure with
`-DCMAKE_INSTALL_BINDIR=. -DCMAKE_INSTALL_DOCDIR=.`). Every archive holds the
compiler, `LICENSE`, `README.md`, `CHANGELOG.md` and a `licenses/` folder with
the license of each library that build embeds.

### Building on Windows

`kriol.exe` builds natively on Windows, and the
[Windows workflow](.github/workflows/windows.yml) publishes it as the
`kriol-windows-x86_64` artifact. Keep `ld.lld.exe` in the same folder as
`kriol.exe`; with it, `kriol hello.kriol` produces `a.exe` without Visual Studio
or any other toolchain installed. The programs it builds run on Windows 7 and
later. `wasm32-wasi` output is disabled on Windows for now.

Windows programs are built against a bundled MinGW runtime taken from an
[llvm-mingw](https://github.com/mstorsjo/llvm-mingw) release (the `msvcrt`
variant). Pointing `KRIOL_MINGW_SYSROOT` at one also enables
`--target x86_64-windows` in Linux builds, which needs `ld.lld` on `PATH`:

```sh
cmake -B build -DKRIOL_MINGW_SYSROOT=/opt/llvm-mingw-20260922-msvcrt-ubuntu-22.04-x86_64
kriol hello.kriol --target x86_64-windows -o hello.exe
```

To build it on Windows, install the
[LLVM 20 `x86_64-pc-windows-msvc` archive](https://github.com/llvm/llvm-project/releases/tag/llvmorg-20.1.8)
(the regular installer lacks the CMake files), an llvm-mingw `msvcrt-x86_64`
release, the Visual Studio Build Tools,
[win_flex_bison](https://github.com/lexxmark/winflexbison), CMake and Ninja.
Then, from a "x64 Native Tools" prompt with the LLVM `bin` folder on `PATH`:

```bat
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=<llvm>\lib\cmake\llvm -DKRIOL_MINGW_SYSROOT=<llvm-mingw>
cmake --build build --target kriol
.\run_tests.ps1 build\kriol.exe .
```

## Tests

Run the suite from the repository root:

```sh
cmake -B build && cmake --build build --target check
```

[`tests/README.md`](./tests/README.md) covers the layout of the suite, the
fixtures, coverage and the fuzzing harnesses. On Windows, the counterpart is
`.\run_tests.ps1 build\kriol.exe .`

## Contributing

Bug reports, test cases and pull requests are all welcome, and the language is
young enough that there is plenty to do. See
[CONTRIBUTING.md](./CONTRIBUTING.md) for how to build the compiler, run the test
suite, and what a change is expected to come with.

For anything large, such as new syntax, a new type or a change to an existing
language rule, please open an issue first so the design can be discussed before
you spend time on it.

## Contributors

Kriol is currently maintained by
[@anaximeno](https://github.com/anaximeno) and [@kidiatoliny](https://github.com/kidiatoliny), with contributions from everyone
below:

<a href="https://github.com/kriol-lang/kriol/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=kriol-lang/kriol&__v=1" alt="Contributors" />
</a>

<!-- The grid above is generated from the commit history by contrib.rocks and
     needs no upkeep. Only the maintainer line is written by hand. -->

## License

All the code and specifications of the KriolLang programming language are currently under the [MIT License](./LICENSE).
