#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static bool primo[1000001];

static int64_t crivo(int64_t n) {
    for (int64_t i = 0; i <= n; i++) {
        primo[i] = true;
    }
    primo[0] = false;
    primo[1] = false;
    for (int64_t i = 2; i * i <= n; i++) {
        if (primo[i]) {
            for (int64_t j = i * i; j <= n; j += i) {
                primo[j] = false;
            }
        }
    }
    int64_t total = 0;
    for (int64_t i = 0; i <= n; i++) {
        if (primo[i]) {
            total++;
        }
    }
    return total;
}

int main(void) {
    int64_t total = 0;
    for (int vez = 0; vez < 10; vez++) {
        total = crivo(1000000);
    }
    printf("%lld\n", (long long) total);
    return 0;
}
