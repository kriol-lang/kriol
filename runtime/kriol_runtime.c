/*
 * KriolLang Runtime Library.
 *
 * Provides small helper functions for the KriolLang runtime, such as printing values.
 */

#include <stdio.h>
#include <float.h>
#include <math.h>
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

/* The program's source file, named in its error messages. The compiler defines
   it in every program; this default only serves code linked without it, such
   as the runtime tests. */
__attribute__((weak)) const char* __kriol_source_name = "kriol";

/* Stops the program with "file:line: category: message", the format of the
   compiler's own messages; the line is left out when it is 0. */
_Noreturn static void __kriol_stop(int line, const char* category, const char* message) {
    __kriol_flush(stdout);
    __kriol_write_text(stderr, __kriol_source_name);
    if (line > 0) {
        char number[16];
        snprintf(number, sizeof number, ":%d", line);
        __kriol_write_text(stderr, number);
    }
    __kriol_write_text(stderr, ": ");
    __kriol_write_text(stderr, category);
    __kriol_write_text(stderr, ": ");
    __kriol_write_text(stderr, message ? message : "");
    __kriol_write_text(stderr, "\n");
    exit(1);
}

#define KRIOL_RUNTIME_ERROR "erro de execução"
#define KRIOL_UNHANDLED_ERROR "erro não tratado"

_Noreturn void __kriol_panic(const char* message) {
    __kriol_stop(0, KRIOL_RUNTIME_ERROR, message);
}

_Noreturn void __kriol_panic_at(const char* message, int line) {
    __kriol_stop(line, KRIOL_RUNTIME_ERROR, message);
}

/* An Erru that a call did not handle: one returned to inisiu, or one of a call
   without 'tenta' or 'sinon'. */
_Noreturn void __kriol_unhandled_error(const char* message, int line) {
    __kriol_stop(line, KRIOL_UNHANDLED_ERROR, message);
}

#if !KRIOL_RUNTIME_NO_GC
static void* __kriol_gc_out_of_memory(size_t requested_bytes) {
    (void)requested_bytes;
    __kriol_panic("a memória do programa esgotou-se");
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
    if (!buf) __kriol_panic("não há memória suficiente para guardar texto");
    return buf;
}

static char* __kriol_resize_text(char* old_buf, size_t bytes) {
#if KRIOL_RUNTIME_NO_GC
    char* buf = (char*)realloc(old_buf, bytes);
#else
    char* buf = (char*)GC_REALLOC(old_buf, bytes);
#endif
    if (!buf) __kriol_panic("não há memória suficiente para guardar texto");
    return buf;
}

#ifdef _WIN32
/* The C runtime would return console input in the legacy code page. */
static char* __kriol_read_console_line(HANDLE console, const char** error) {
    size_t cap = 128;
    size_t len = 0;
    WCHAR* wide = (WCHAR*)malloc(cap * sizeof(WCHAR));
    if (!wide) __kriol_panic("não há memória suficiente para ler a entrada");

    for (;;) {
        if (len == cap) {
            cap *= 2;
            WCHAR* grown = (WCHAR*)realloc(wide, cap * sizeof(WCHAR));
            if (!grown) __kriol_panic("não há memória suficiente para ler a entrada");
            wide = grown;
        }

        DWORD read = 0;
        if (!ReadConsoleW(console, wide + len, (DWORD)(cap - len), &read, NULL)) {
            free(wide);
            *error = "não foi possível ler a entrada";
            return NULL;
        }
        if (read == 0)
            break;
        len += read;
        if (wide[len - 1] == L'\n')
            break;
    }

    /* Ctrl+Z at the start of a line is the console's end of input. */
    if (len == 0 || wide[0] == 0x1A) {
        free(wide);
        *error = "a entrada terminou e não há mais linhas para ler";
        return NULL;
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

/* Reads one line. On end of input without data, or on a read failure, returns
 * NULL and stores a message in *error. */
char* __kriol_read_line(const char* prompt, const char** error) {
    if (prompt && prompt[0] != '\0')
        __kriol_write_text(stdout, prompt);
    __kriol_flush(stdout);

#ifdef _WIN32
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (__kriol_is_console(input))
        return __kriol_read_console_line(input, error);
#endif

    size_t cap = 128;
    size_t len = 0;
    char* buf = __kriol_alloc_text(cap);

    int ch;
    int read_any = 0;
    while ((ch = fgetc(stdin)) != EOF) {
        read_any = 1;
        if (ch == '\n') break;
        if (len + 1 >= cap) {
            if (cap > ((size_t)-1) / 2)
                __kriol_panic("a linha lida é demasiado grande");
            cap *= 2;
            buf = __kriol_resize_text(buf, cap);
        }
        buf[len++] = (char)ch;
    }

    if (ferror(stdin)) {
        *error = "não foi possível ler a entrada";
        return NULL;
    }
    if (!read_any) {
        *error = "a entrada terminou e não há mais linhas para ler";
        return NULL;
    }

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
        __kriol_panic("não foi possível formatar o texto");

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
                __kriol_panic("o texto é demasiado grande");
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

static void __kriol_append_element(KriolTextBuilder* builder, const unsigned char* element,
                                   int32_t kind, int32_t bits) {
    char number[64];
    switch (kind) {
        case KRIOL_ELEMENT_SIGNED:
            snprintf(number, sizeof number, "%lld", __kriol_read_signed(element, bits));
            __kriol_builder_append_text(builder, number);
            break;
        case KRIOL_ELEMENT_UNSIGNED:
            snprintf(number, sizeof number, "%llu", __kriol_read_unsigned(element, bits));
            __kriol_builder_append_text(builder, number);
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
            __kriol_builder_append_text(builder, number);
            break;
        }
        case KRIOL_ELEMENT_BOOL:
            __kriol_builder_append_text(builder, __kriol_bool_to_string(*element != 0));
            break;
        default: {
            const char* text;
            memcpy(&text, element, sizeof text);
            if (text)
                __kriol_builder_append_text(builder, text);
            break;
        }
    }
}

/* Appends `[a, b, ...]` for an array with dimensions dims[0..rank), the
 * outermost first, laid out row after row. */
static void __kriol_append_array(KriolTextBuilder* builder, const unsigned char* data,
                                 const int64_t* dims, int32_t rank, int32_t kind, int32_t bits,
                                 size_t stride) {
    size_t step = stride;
    for (int32_t d = 1; d < rank; ++d)
        step *= (size_t)dims[d];

    __kriol_builder_append(builder, "[", 1);
    for (int64_t i = 0; i < dims[0]; ++i) {
        if (i > 0)
            __kriol_builder_append(builder, ", ", 2);
        const unsigned char* element = data + (size_t)i * step;
        if (rank > 1)
            __kriol_append_array(builder, element, dims + 1, rank - 1, kind, bits, stride);
        else
            __kriol_append_element(builder, element, kind, bits);
    }
    __kriol_builder_append(builder, "]", 1);
}

char* __kriol_array_to_text(const void* data, const int64_t* dims, int32_t rank, int32_t kind, int32_t bits) {
    size_t stride = kind == KRIOL_ELEMENT_TEXT ? sizeof(const char*)
                  : kind == KRIOL_ELEMENT_BOOL ? 1
                  : (size_t)bits / 8;
    KriolTextBuilder builder = { __kriol_alloc_text(64), 0, 64 };
    __kriol_append_array(&builder, (const unsigned char*)data, dims, rank, kind, bits, stride);
    builder.Data[builder.Len] = '\0';
    return builder.Data;
}

/* ---- text to number conversion: T::konverti(text) ----------------------------
 * Each function returns 1 and stores the value on success. On failure it
 * returns 0 and stores a message in *error. Surrounding white space is ignored. */

/* Error messages quote at most this many bytes of the input. */
#define KRIOL_QUOTE_MAX 40

static int __kriol_quote_length(const char* text) {
    size_t cut = strlen(text);
    if (cut <= KRIOL_QUOTE_MAX) return (int)cut;
    cut = KRIOL_QUOTE_MAX;
    while (cut > 0 && ((unsigned char)text[cut] & 0xC0) == 0x80) --cut;  /* UTF-8 boundary */
    return (int)cut;
}

/* The arguments for a "%.*s%s" that quotes `text`, cut short when long. */
#define KRIOL_QUOTE(text) __kriol_quote_length(text), (text), (strlen(text) > KRIOL_QUOTE_MAX ? "..." : "")

static int __kriol_is_space(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static void __kriol_trim(const char* text, const char** begin, const char** end) {
    const char* start = text;
    while (__kriol_is_space(*start)) ++start;
    const char* stop = start + strlen(start);
    while (stop > start && __kriol_is_space(stop[-1])) --stop;
    *begin = start;
    *end = stop;
}

int __kriol_convert_int(const char* text, int64_t* out, int32_t bits, int32_t is_signed, const char** error) {
    const char* p;
    const char* end;
    __kriol_trim(text, &p, &end);

    int negative = 0;
    if (p < end && (*p == '+' || *p == '-')) {
        negative = (*p == '-');
        ++p;
    }
    if (p == end) {
        *error = __kriol_format("'%.*s%s' não é um número inteiro válido", KRIOL_QUOTE(text));
        return 0;
    }

    uint64_t magnitude = 0;
    int overflow = 0;
    for (; p < end; ++p) {
        if (*p < '0' || *p > '9') {
            *error = __kriol_format("'%.*s%s' não é um número inteiro válido", KRIOL_QUOTE(text));
            return 0;
        }
        unsigned digit = (unsigned)(*p - '0');
        if (magnitude > (UINT64_MAX - digit) / 10)
            overflow = 1;
        else
            magnitude = magnitude * 10 + digit;
    }

    uint64_t limit;
    if (is_signed)
        limit = negative ? ((uint64_t)1 << (bits - 1)) : ((uint64_t)1 << (bits - 1)) - 1;
    else
        limit = bits >= 64 ? UINT64_MAX : (((uint64_t)1 << bits) - 1);
    if (!is_signed && negative && magnitude != 0)
        overflow = 1;

    if (overflow || magnitude > limit) {
        /* int is the name a program uses for i64. */
        char type[8];
        if (is_signed && bits == 64)
            snprintf(type, sizeof type, "int");
        else
            snprintf(type, sizeof type, "%c%d", is_signed ? 'i' : 'u', (int)bits);
        *error = __kriol_format("'%.*s%s' não cabe no tipo '%s'", KRIOL_QUOTE(text), type);
        return 0;
    }

    *out = (is_signed && negative) ? (int64_t)(0 - magnitude) : (int64_t)magnitude;
    return 1;
}

int __kriol_convert_float(const char* text, double* out, int32_t bits, const char** error) {
    const char* begin;
    const char* end;
    __kriol_trim(text, &begin, &end);

    /* [+-] digits [. digits] [(e|E) [+-] digits], and nothing else. */
    const char* p = begin;
    if (p < end && (*p == '+' || *p == '-')) ++p;
    const char* digits = p;
    while (p < end && *p >= '0' && *p <= '9') ++p;
    int valid = p > digits;
    if (valid && p < end && *p == '.') {
        ++p;
        const char* fraction = p;
        while (p < end && *p >= '0' && *p <= '9') ++p;
        valid = p > fraction;
    }
    if (valid && p < end && (*p == 'e' || *p == 'E')) {
        ++p;
        if (p < end && (*p == '+' || *p == '-')) ++p;
        const char* exponent = p;
        while (p < end && *p >= '0' && *p <= '9') ++p;
        valid = p > exponent;
    }
    if (!valid || p != end) {
        *error = __kriol_format("'%.*s%s' não é um número válido", KRIOL_QUOTE(text));
        return 0;
    }

    double value = strtod(begin, NULL);
    if (isinf(value) || (bits == 32 && fabs(value) > FLT_MAX)) {
        *error = __kriol_format("o número '%.*s%s' é demasiado grande", KRIOL_QUOTE(text));
        return 0;
    }
    *out = value;
    return 1;
}

int __kriol_convert_bool(const char* text, int32_t* out, const char** error) {
    const char* begin;
    const char* end;
    __kriol_trim(text, &begin, &end);
    size_t length = (size_t)(end - begin);

    if (length == 3 && strncmp(begin, "sin", 3) == 0) {
        *out = 1;
        return 1;
    }
    if (length == 3 && strncmp(begin, "nau", 3) == 0) {
        *out = 0;
        return 1;
    }
    *error = __kriol_format("'%.*s%s' não é um valor lógico válido; esperava-se sin ou nau", KRIOL_QUOTE(text));
    return 0;
}

void __kriol_assert_message(int cond, int line, const char* message) {
    if (!cond)
        __kriol_panic_at(message && message[0] != '\0' ? message : "a condição de konfirma é falsa", line);
}

void __kriol_assert(int cond, int line) {
    __kriol_assert_message(cond, line, NULL);
}

void __kriol_check_bounds(int64_t index, int64_t size, int line) {
    if (index < 0 || index >= size) {
        char message[96];
        snprintf(message, sizeof message, "o índice %lld está fora do array, que vai de 0 a %lld",
                 (long long)index, (long long)size - 1);
        __kriol_panic_at(message, line);
    }
}

void __kriol_check_fdiv(double divisor, int32_t line) {
    if (divisor == 0.0)
        __kriol_panic_at("divisão por zero", line);
}

void __kriol_check_div(int64_t lhs, int64_t rhs, int32_t signed_bits, int32_t line) {
    if (rhs == 0)
        __kriol_panic_at("divisão por zero", line);

    if (signed_bits > 0 && rhs == -1) {
        int64_t min = signed_bits >= 64
            ? INT64_MIN
            : -((int64_t)1 << (signed_bits - 1));
        if (lhs == min)
            __kriol_panic_at("o resultado da divisão não cabe no tipo do número inteiro", line);
    }
}
