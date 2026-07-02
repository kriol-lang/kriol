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
// NOTE: Injected in the linking phase of the kriol compilation process.
#include "gc.h"
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

void __kriol_print_i64(int64_t v) {
    printf("%lld", (long long)v);
}

void __kriol_print_u64(uint64_t v) {
    printf("%llu", (unsigned long long)v);
}

void __kriol_print_f64(double v) {
    printf("%g", v);
}

const char* __kriol_bool_to_string(int v) {
    return v ? "sin" : "nau";
}

void __kriol_print_bool(int v) {
    fputs(__kriol_bool_to_string(v), stdout);
}

void __kriol_print_string(const char* s) {
    if (s) fputs(s, stdout);
}

void __kriol_println_i64(int64_t v) {
    __kriol_print_i64(v);
    putchar('\n');
}

void __kriol_println_u64(uint64_t v) {
    __kriol_print_u64(v);
    putchar('\n');
}

void __kriol_println_f64(double v) {
    __kriol_print_f64(v);
    putchar('\n');
}

void __kriol_println_bool(int v) {
    __kriol_print_bool(v);
    putchar('\n');
}

void __kriol_println_string(const char* s) {
    __kriol_print_string(s);
    putchar('\n');
}

static char* __kriol_alloc_text(size_t bytes) {
#if KRIOL_RUNTIME_NO_GC
    char* buf = (char*)malloc(bytes);
#else
    char* buf = (char*)GC_MALLOC_ATOMIC(bytes);
#endif
    if (!buf) __kriol_panic("out of memory while allocating memory for text data");
    return buf;
}

static char* __kriol_resize_text(char* old_buf, size_t bytes) {
#if KRIOL_RUNTIME_NO_GC
    char* buf = (char*)realloc(old_buf, bytes);
#else
    char* buf = (char*)GC_REALLOC(old_buf, bytes);
#endif
    if (!buf) __kriol_panic("out of memory while allocating memory for text data");
    return buf;
}

char* __kriol_read_line(const char* prompt) {
    if (prompt && prompt[0] != '\0') {
        fputs(prompt, stdout);
        fflush(stdout);
    }

    size_t cap = 128;
    size_t len = 0;
    char* buf = __kriol_alloc_text(cap);

    int ch;
    while ((ch = fgetc(stdin)) != EOF) {
        if (ch == '\n') break;
        if (len + 1 >= cap) {
            if (cap > ((size_t)-1) / 2)
                __kriol_panic("input line is too large");
            cap *= 2;
            buf = __kriol_resize_text(buf, cap);
        }
        buf[len++] = (char)ch;
    }

    if (ferror(stdin))
        __kriol_panic("failed to read from stdin");

    if (len > 0 && buf[len - 1] == '\r')
        --len;

    buf[len] = '\0';
    return buf;
}

char* __kriol_format(const char* fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    // vsnprintf returns the number of characters that would have been written,
    // excluding null terminator. If needed is negative, an encoding error occurred.
    if (needed < 0) return NULL;

    char* buf = __kriol_alloc_text((size_t)needed + 1);

    va_start(args, fmt);
    int written = vsnprintf(buf, (size_t)needed + 1, fmt, args);
    va_end(args);

    if (written < 0 || written != needed)
        __kriol_panic("failed to format text");

    return buf;
}

void __kriol_assert_message(int cond, int line, const char* message) {
    if (!cond)
        __kriol_panic_at(message && message[0] != '\0' ? message : "assertion failed", line);
}

void __kriol_assert(int cond, int line) {
    __kriol_assert_message(cond, line, NULL);
}

void __kriol_check_bounds(int64_t index, int64_t size, int line) {
    if (index < 0 || index >= size) {
        fprintf(stderr, "kriol: array index out of bounds at line %d: %lld not in [0..%lld]\n",
                line, (long long)index, (long long)size - 1);
        exit(1);
    }
}

void __kriol_check_div(int64_t lhs, int64_t rhs, int32_t signed_bits, int32_t line) {
    if (rhs == 0)
        __kriol_panic_at("division by zero", line);

    if (signed_bits > 0 && rhs == -1) {
        int64_t min = signed_bits >= 64
            ? INT64_MIN
            : -((int64_t)1 << (signed_bits - 1));
        if (lhs == min)
            __kriol_panic_at("integer overflow in division", line);
    }
}
