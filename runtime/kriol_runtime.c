/*
 * KriolLang Runtime Library.
 *
 * Provides small helper functions for the KriolLang runtime, such as printing values.
 */

#include <stdio.h>
#include <stdint.h>

void __kriol_print_nter(int64_t v) {
    printf("%lld", (long long)v);
}

void __kriol_print_num(double v) {
    printf("%g", v);
}

void __kriol_print_bool(int v) {
    fputs(v ? "sin" : "nau", stdout);
}

void __kriol_print_textu(const char* s) {
    if (s) fputs(s, stdout);
}

void __kriol_println_nter(int64_t v) {
    __kriol_print_nter(v);
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
