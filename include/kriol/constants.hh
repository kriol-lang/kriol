#ifndef _KRIOL_CNST_HEADER
#define _KRIOL_CNST_HEADER

#define KR_VERSION_MAJOR 1
#define KR_VERSION_MINOR 9
#define KR_VERSION_PATCH 2

#define KR_STANDARD_NAME "Kriol"
#define KR_STANDARD_FILE_EXTENSION "kriol"
#define KR_ALTERNATIVE_FILE_EXTENSION "kr"
#define KR_STANDARD_COMPILER_NAME "kriol"

#define KR_VERSION_STRING \
    KR_STANDARD_NAME " v" KR_XSTR(KR_VERSION_MAJOR) "." \
    KR_XSTR(KR_VERSION_MINOR) "." KR_XSTR(KR_VERSION_PATCH)
#define KR_XSTR(x) KR_STR(x)
#define KR_STR(x) #x

#define KR_DEFAULT_WINDOWS_OUT_FILE "a.exe"
#ifdef _WIN32
#define KR_DEFAULT_OUT_FILE KR_DEFAULT_WINDOWS_OUT_FILE
#else
#define KR_DEFAULT_OUT_FILE "a.out"
#endif
#define KR_DEFAULT_WASM_OUT_FILE "a.wasm"

// Maximum number of elements in a fixed-size array declaration.
// Keeps pathological sizes from exhausting compiler memory or hanging
// codegen (e.g. repeat initializers are unrolled element by element).
#define KR_MAX_ARRAY_SIZE (1u << 20)

// Maximum nesting depth of one expression, counting operators, calls,
// brackets and element initializers. Sema, codegen and the AST destructors
// recurse over expression trees; long operator chains nest as deeply as
// brackets do (same default as clang's -fbracket-depth).
#define KR_MAX_EXPR_DEPTH 256

// Largest local variable or parameter, in bytes. Locals live on the stack,
// which kriol programs get 8 MiB of on every target; bigger values belong in
// top-level variables, which live in static storage.
#define KR_MAX_LOCAL_BYTES (1u << 20)

#endif // _KRIOL_CNST_HEADER