/*
 * KriolLang Runtime Library.
 *
 * Provides small helper functions for the KriolLang runtime, such as printing values.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

#ifndef KRIOL_RUNTIME_NO_GC
#define KRIOL_RUNTIME_NO_GC 0
#endif

#if !KRIOL_RUNTIME_NO_GC
#include <gc.h>
#endif

_Noreturn void __kriol_panic(const char* message) {
    fputs("kriol: panic", stderr);
    if (message && message[0] != '\0') {
        fputs(": ", stderr);
        fputs(message, stderr);
    }
    putc('\n', stderr);
    exit(1);
}

_Noreturn void __kriol_panic_at(const char* message, int line) {
    fprintf(stderr, "kriol: panic at line %d", line);
    if (message && message[0] != '\0') {
        fputs(": ", stderr);
        fputs(message, stderr);
    }
    putc('\n', stderr);
    exit(1);
}

#if !KRIOL_RUNTIME_NO_GC
static void* __kriol_gc_out_of_memory(size_t requested_bytes) {
    (void)requested_bytes;
    __kriol_panic("garbage-collected heap exhausted");
}
#endif

void __kriol_gc_init(void) {
#if !KRIOL_RUNTIME_NO_GC
    GC_set_oom_fn(__kriol_gc_out_of_memory);
    GC_INIT();
#endif
}

void __kriol_print_nter(int64_t v) {
    printf("%lld", (long long)v);
}

void __kriol_print_unter(uint64_t v) {
    printf("%llu", (unsigned long long)v);
}

void __kriol_print_num(double v) {
    printf("%g", v);
}

const char* __kriol_bool_to_str(int v) {
    return v ? "sin" : "nau";
}

void __kriol_print_bool(int v) {
    fputs(__kriol_bool_to_str(v), stdout);
}

void __kriol_print_textu(const char* s) {
    if (s) fputs(s, stdout);
}

void __kriol_println_nter(int64_t v) {
    __kriol_print_nter(v);
    putchar('\n');
}

void __kriol_println_unter(uint64_t v) {
    __kriol_print_unter(v);
    putchar('\n');
}

void __kriol_println_num(double v) {
    __kriol_print_num(v);
    putchar('\n');
}

void __kriol_println_bool(int v) {
    __kriol_print_bool(v);
    putchar('\n');
}

void __kriol_println_textu(const char* s) {
    __kriol_print_textu(s);
    putchar('\n');
}

char* __kriol_format(const char* fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    // vsnprintf returns the number of characters that would have been written,
    // excluding null terminator. If needed is negative, an encoding error occurred.
    if (needed < 0) return NULL;

#if KRIOL_RUNTIME_NO_GC
    char* buf = (char*)malloc((size_t)needed + 1);
#else
    char* buf = (char*)GC_MALLOC_ATOMIC((size_t)needed + 1);
#endif

    if (!buf) __kriol_panic("out of memory while formatting text");

    va_start(args, fmt);
    int written = vsnprintf(buf, (size_t)needed + 1, fmt, args);
    va_end(args);

    if (written < 0 || written != needed)
        __kriol_panic("failed to format text");

    return buf;
}

void __kriol_konfirma_message(int cond, int line, const char* message) {
    if (!cond)
        __kriol_panic_at(message && message[0] != '\0' ? message : "konfirma failed", line);
}

void __kriol_konfirma(int cond, int line) {
    __kriol_konfirma_message(cond, line, NULL);
}

void __kriol_check_bounds(int64_t index, int64_t size, int line) {
    if (index < 0 || index >= size) {
        fprintf(stderr, "kriol: array index out of bounds at line %d: %lld not in [0..%lld]\n",
                line, (long long)index, (long long)size - 1);
        exit(1);
    }
}
