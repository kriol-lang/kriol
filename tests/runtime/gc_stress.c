#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <gc.h>

void __kriol_gc_init(void);
char* __kriol_format(const char* fmt, ...);

int main(void) {
    enum { retained_count = 32, allocation_count = 100000 };
    char* retained[retained_count];

    __kriol_gc_init();

    for (int i = 0; i < allocation_count; ++i) {
        char* formatted = __kriol_format("temporary-%d", i);
        assert(formatted != NULL);

        if (i < retained_count)
            retained[i] = formatted;

        if (i > 0 && i % 10000 == 0)
            GC_gcollect();
    }

    GC_gcollect();

    for (int i = 0; i < retained_count; ++i) {
        char expected[32];
        int written = snprintf(expected, sizeof(expected), "temporary-%d", i);
        assert(written > 0 && (size_t)written < sizeof(expected));
        assert(strcmp(retained[i], expected) == 0);
    }

    return 0;
}
