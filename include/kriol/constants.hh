#ifndef _KRIOL_CNST_HEADER
#define _KRIOL_CNST_HEADER

#define KR_VERSION_MAJOR 1
#define KR_VERSION_MINOR 11
#define KR_VERSION_PATCH 0

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

// Bounds the recursion of sema, codegen and AST destructors. Operator chains
// such as 1 + 1 + ... count one level per operator.
#define KR_MAX_EXPR_DEPTH 256

// Largest local variable or parameter, in bytes; programs get an 8 MiB stack.
#define KR_MAX_LOCAL_BYTES (1u << 20)

#endif // _KRIOL_CNST_HEADER