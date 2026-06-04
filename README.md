# 🦈 KriolLang

**KriolLang** or **Kriol** is a programming language based on the **Cape Verdean Creole**. It was made to be easy for anyone who understands Cape Verdean Creole while keeping the same versatility as some other languages, such as C, Go, and Rust.

```kriol
fn ola(textu nomi, bool naKriolu) {
    si naKriolu {
        mostran(f"Olá {nomi}, ami nta programa na Kriolu!");
    } sinon {
        mostran(f"Hello {nomi}, I program in Kriol!");
    }
}

fn inisiu() {
    ola("Visitanti", sin);
    ola("Guest", nau);
}
```

### Description

The syntax of this programming language was initially based on C, Go, and Rust, but other programming languages such as Liquid played a good role in the initial design.

For more concrete specifications about the language, you are recommended to read the [KriolLang Language Specification](./docs/kriol-lang-spec.md).

The file extension of the KriolLang programming language is `.kl` or `.kriol` and the name of the lang's compiler is **kriol**.

## Installation

It isn't directly installable on any OS yet, but it still can be tested using a Unix-based operating system. It depends on **clang-19**, **clang++-19**, and **llvm-19** to compile the source code and link user programs, and **bison** and **flex** to compile the language rules.

If you are on a Debian (or Ubuntu) based Linux operating system you can install the dependencies using the following command:

```sh
apt install make flex bison clang-19 clang++-19 llvm-19 llvm-19-dev libgc-dev xxd
```

NOTE: now pre-generated parser and scanner codes are provided in the repository, so, **flex** and **bison** can be omitted from the installation above, but if you are planning to make changes to the parser or scanner rules, they will still be necessary.

After installing the dependencies, compile the compiler using:

```sh
cmake -B build && cmake --build build
```

For a release build use:

```sh
cmake -B build/release -DCMAKE_BUILD_TYPE=Release -DKRIOL_STATIC=ON && cmake --build build/release
```

The result of the compilation will be an executable named **kriol** which you can execute using:

```sh
./kriol
```

## Usage

The first thing to do after compiling the compiler is to execute the help command:

```sh
./kriol --help
```

After executing that command you may see the usage section and some of its options.

An example of how it could be used is to compile one of the example files in the examples folder:

```sh
./kriol examples/fibonacci-recursive.kl
```

This compiles `fibonacci-recursive.kl` to a native binary named `a.out` by default. Use `-o` to specify a different output name:

```sh
./kriol examples/fibonacci-recursive.kl -o fibonacci-recursive
```

Then execute the compiled binary with:

```sh
./fibonacci-recursive
```

To inspect the generated LLVM IR instead of producing a binary:

```sh
./kriol examples/example03.kl --emit-ir
```

## Tests

You can run tests with the following command:

```sh
cmake -B build && cmake --build build --target check
```

<!--
## Visuals

TODO...

## Contributing

TODO...

-->

## License

All the code and specifications of the KriolLang programming language are currently under the [MIT License](./LICENSE).
