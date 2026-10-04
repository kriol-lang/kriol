/*
 * KriolLang Runtime Library.
 *
 * Provides small helper functions for the KriolLang runtime, such as printing values.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifndef KRIOL_RUNTIME_NO_GC
#define KRIOL_RUNTIME_NO_GC 0
#endif

#if !KRIOL_RUNTIME_NO_GC
// NOTE: Injected in the linking phase of the kriol compilation process.
#include "gc.h"
#endif

/*
 * All output must go through __kriol_write: Windows consoles get UTF-16 via
 * WriteConsoleW, since UTF-8 written through the C runtime is garbled before
 * Windows 10 whatever the code page.
 */

#ifdef _WIN32
typedef struct {
    int Checked;
    HANDLE Console;             /* NULL when the stream is not a console */
    size_t Len;
    char Pending[4096];         /* UTF-8 held until a newline or a full buffer */
} KriolConsoleStream;

static KriolConsoleStream __kriol_console_streams[2]; /* stdout, stderr */

static int __kriol_is_console(HANDLE handle) {
    DWORD mode;
    return handle && handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &mode);
}

static KriolConsoleStream* __kriol_console_stream(FILE* stream) {
    int is_stderr = stream == stderr;
    KriolConsoleStream* console = &__kriol_console_streams[is_stderr];
    if (!console->Checked) {
        HANDLE handle = GetStdHandle(is_stderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
        console->Console = __kriol_is_console(handle) ? handle : NULL;
        console->Checked = 1;
    }
    return console->Console ? console : NULL;
}

/* Length of the longest prefix of `data` that does not end mid-character. */
static size_t __kriol_utf8_complete_prefix(const char* data, size_t len) {
    size_t lead = len;
    size_t continuation = 0;
    while (lead > 0 && continuation < 4 && ((unsigned char)data[lead - 1] & 0xC0) == 0x80) {
        --lead;
        ++continuation;
    }
    if (lead == 0)
        return len;

    unsigned char byte = (unsigned char)data[lead - 1];
    size_t needed = (byte & 0xE0) == 0xC0 ? 2
                  : (byte & 0xF0) == 0xE0 ? 3
                  : (byte & 0xF8) == 0xF0 ? 4
                  : 1;
    return continuation + 1 >= needed ? len : lead - 1;
}

static void __kriol_console_flush(KriolConsoleStream* console) {
    size_t complete = __kriol_utf8_complete_prefix(console->Pending, console->Len);
    if (complete == 0)
        return;

    /* UTF-16 never needs more code units than UTF-8 needs bytes. */
    WCHAR wide[sizeof console->Pending];
    int count = MultiByteToWideChar(CP_UTF8, 0, console->Pending, (int)complete,
                                    wide, (int)(sizeof wide / sizeof wide[0]));
    DWORD written;
    if (count > 0)
        WriteConsoleW(console->Console, wide, (DWORD)count, &written, NULL);

    memmove(console->Pending, console->Pending + complete, console->Len - complete);
    console->Len -= complete;
}
#endif

static void __kriol_write(FILE* stream, const char* data, size_t len) {
#ifdef _WIN32
    KriolConsoleStream* console = __kriol_console_stream(stream);
    if (console) {
        for (size_t i = 0; i < len; ++i) {
            console->Pending[console->Len++] = data[i];
            if (data[i] == '\n' || console->Len == sizeof console->Pending)
                __kriol_console_flush(console);
        }
        if (stream == stderr)
            __kriol_console_flush(console);
        return;
    }
#endif
    fwrite(data, 1, len, stream);
}

static void __kriol_flush(FILE* stream) {
#ifdef _WIN32
    KriolConsoleStream* console = __kriol_console_stream(stream);
    if (console) {
        __kriol_console_flush(console);
        return;
    }
#endif
    fflush(stream);
}

static void __kriol_flush_all(void) {
    __kriol_flush(stdout);
    __kriol_flush(stderr);
}

static void __kriol_write_text(FILE* stream, const char* text) {
    __kriol_write(stream, text, strlen(text));
}

static void __kriol_write_format(FILE* stream, const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);
    if (len > 0)
        __kriol_write(stream, buf, (size_t)len < sizeof buf ? (size_t)len : sizeof buf - 1);
}

_Noreturn static void __kriol_fail(const char* prefix, const char* message) {
    __kriol_flush(stdout);
    __kriol_write_text(stderr, prefix);
    if (message && message[0] != '\0') {
        __kriol_write_text(stderr, ": ");
        __kriol_write_text(stderr, message);
    }
    __kriol_write_text(stderr, "\n");
    exit(1);
}

_Noreturn void __kriol_panic(const char* message) {
    __kriol_fail("kriol: panic", message);
}

// An error that reached inisiu without being handled.
_Noreturn void __kriol_unhandled_error(const char* message) {
    __kriol_fail("kriol: err", message);
}

_Noreturn void __kriol_panic_at(const char* message, int line) {
    char prefix[64];
    snprintf(prefix, sizeof prefix, "kriol: panic at line %d", line);
    __kriol_fail(prefix, message);
}

#if !KRIOL_RUNTIME_NO_GC
static void* __kriol_gc_out_of_memory(size_t requested_bytes) {
    (void)requested_bytes;
    __kriol_panic("garbage-collected heap exhausted");
}
#endif

void __kriol_gc_init(void) {
    atexit(__kriol_flush_all);
#if !KRIOL_RUNTIME_NO_GC
    GC_set_oom_fn(__kriol_gc_out_of_memory);
    GC_INIT();
#endif
}

void __kriol_print_char(int c) {
    char ch = (char)c;
    __kriol_write(stdout, &ch, 1);
}

void __kriol_print_i64(int64_t v) {
    __kriol_write_format(stdout, "%lld", (long long)v);
}

void __kriol_print_u64(uint64_t v) {
    __kriol_write_format(stdout, "%llu", (unsigned long long)v);
}

void __kriol_print_f64(double v) {
    __kriol_write_format(stdout, "%g", v);
}

const char* __kriol_bool_to_string(int v) {
    return v ? "sin" : "nau";
}

void __kriol_print_bool(int v) {
    __kriol_write_text(stdout, __kriol_bool_to_string(v));
}

void __kriol_print_string(const char* s) {
    if (s) __kriol_write_text(stdout, s);
}

void __kriol_println_i64(int64_t v) {
    __kriol_print_i64(v);
    __kriol_print_char('\n');
}

void __kriol_println_u64(uint64_t v) {
    __kriol_print_u64(v);
    __kriol_print_char('\n');
}

void __kriol_println_f64(double v) {
    __kriol_print_f64(v);
    __kriol_print_char('\n');
}

void __kriol_println_bool(int v) {
    __kriol_print_bool(v);
    __kriol_print_char('\n');
}

void __kriol_println_string(const char* s) {
    __kriol_print_string(s);
    __kriol_print_char('\n');
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

#ifdef _WIN32
/* The C runtime would return console input in the legacy code page. */
static char* __kriol_read_console_line(HANDLE console) {
    size_t cap = 128;
    size_t len = 0;
    WCHAR* wide = (WCHAR*)malloc(cap * sizeof(WCHAR));
    if (!wide) __kriol_panic("out of memory while reading from stdin");

    for (;;) {
        if (len == cap) {
            cap *= 2;
            WCHAR* grown = (WCHAR*)realloc(wide, cap * sizeof(WCHAR));
            if (!grown) __kriol_panic("out of memory while reading from stdin");
            wide = grown;
        }

        DWORD read = 0;
        if (!ReadConsoleW(console, wide + len, (DWORD)(cap - len), &read, NULL))
            __kriol_panic("failed to read from stdin");
        if (read == 0)
            break;
        len += read;
        if (wide[len - 1] == L'\n')
            break;
    }

    for (size_t i = 0; i < len; ++i) {
        if (wide[i] == 0x1A) {
            len = i;
            break;
        }
    }
    while (len > 0 && (wide[len - 1] == L'\n' || wide[len - 1] == L'\r'))
        --len;

    int bytes = len == 0 ? 0 : WideCharToMultiByte(CP_UTF8, 0, wide, (int)len, NULL, 0, NULL, NULL);
    char* text = __kriol_alloc_text((size_t)bytes + 1);
    if (bytes > 0)
        WideCharToMultiByte(CP_UTF8, 0, wide, (int)len, text, bytes, NULL, NULL);
    text[bytes] = '\0';
    free(wide);
    return text;
}
#endif

char* __kriol_read_line(const char* prompt) {
    if (prompt && prompt[0] != '\0')
        __kriol_write_text(stdout, prompt);
    __kriol_flush(stdout);

#ifdef _WIN32
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (__kriol_is_console(input))
        return __kriol_read_console_line(input);
#endif

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

/* Must match codegen's ArrayElementKind. */
enum {
    KRIOL_ELEMENT_SIGNED = 0,
    KRIOL_ELEMENT_UNSIGNED = 1,
    KRIOL_ELEMENT_FLOAT = 2,
    KRIOL_ELEMENT_BOOL = 3,
    KRIOL_ELEMENT_TEXT = 4
};

typedef struct {
    char* Data;
    size_t Len;
    size_t Cap;
} KriolTextBuilder;

static void __kriol_builder_append(KriolTextBuilder* builder, const char* text, size_t len) {
    if (builder->Len + len + 1 > builder->Cap) {
        size_t cap = builder->Cap;
        while (builder->Len + len + 1 > cap) {
            if (cap > ((size_t)-1) / 2)
                __kriol_panic("text is too large");
            cap *= 2;
        }
        builder->Data = __kriol_resize_text(builder->Data, cap);
        builder->Cap = cap;
    }
    memcpy(builder->Data + builder->Len, text, len);
    builder->Len += len;
}

static void __kriol_builder_append_text(KriolTextBuilder* builder, const char* text) {
    __kriol_builder_append(builder, text, strlen(text));
}

static long long __kriol_read_signed(const unsigned char* element, int32_t bits) {
    switch (bits) {
        case 8:  { int8_t v;  memcpy(&v, element, sizeof v); return v; }
        case 16: { int16_t v; memcpy(&v, element, sizeof v); return v; }
        case 32: { int32_t v; memcpy(&v, element, sizeof v); return v; }
        default: { int64_t v; memcpy(&v, element, sizeof v); return v; }
    }
}

static unsigned long long __kriol_read_unsigned(const unsigned char* element, int32_t bits) {
    switch (bits) {
        case 8:  { uint8_t v;  memcpy(&v, element, sizeof v); return v; }
        case 16: { uint16_t v; memcpy(&v, element, sizeof v); return v; }
        case 32: { uint32_t v; memcpy(&v, element, sizeof v); return v; }
        default: { uint64_t v; memcpy(&v, element, sizeof v); return v; }
    }
}

char* __kriol_array_to_text(const void* data, int64_t count, int32_t kind, int32_t bits) {
    size_t stride = kind == KRIOL_ELEMENT_TEXT ? sizeof(const char*)
                  : kind == KRIOL_ELEMENT_BOOL ? 1
                  : (size_t)bits / 8;
    KriolTextBuilder builder = { __kriol_alloc_text(64), 0, 64 };
    char number[64];

    __kriol_builder_append(&builder, "[", 1);
    for (int64_t i = 0; i < count; ++i) {
        const unsigned char* element = (const unsigned char*)data + (size_t)i * stride;
        if (i > 0)
            __kriol_builder_append(&builder, ", ", 2);

        switch (kind) {
            case KRIOL_ELEMENT_SIGNED:
                snprintf(number, sizeof number, "%lld", __kriol_read_signed(element, bits));
                __kriol_builder_append_text(&builder, number);
                break;
            case KRIOL_ELEMENT_UNSIGNED:
                snprintf(number, sizeof number, "%llu", __kriol_read_unsigned(element, bits));
                __kriol_builder_append_text(&builder, number);
                break;
            case KRIOL_ELEMENT_FLOAT: {
                double v;
                if (bits == 32) {
                    float f;
                    memcpy(&f, element, sizeof f);
                    v = f;
                } else {
                    memcpy(&v, element, sizeof v);
                }
                snprintf(number, sizeof number, "%g", v);
                __kriol_builder_append_text(&builder, number);
                break;
            }
            case KRIOL_ELEMENT_BOOL:
                __kriol_builder_append_text(&builder, __kriol_bool_to_string(*element != 0));
                break;
            default: {
                const char* text;
                memcpy(&text, element, sizeof text);
                if (text)
                    __kriol_builder_append_text(&builder, text);
                break;
            }
        }
    }
    __kriol_builder_append(&builder, "]", 1);

    builder.Data[builder.Len] = '\0';
    return builder.Data;
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
        __kriol_flush(stdout);
        __kriol_write_format(stderr, "kriol: array index out of bounds at line %d: %lld not in [0..%lld]\n",
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
