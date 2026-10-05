#include <stdint.h>
#include <stdio.h>

static int64_t fibo(int64_t n) {
    if (n < 2) {
        return n;
    }
    return fibo(n - 1) + fibo(n - 2);
}

int main(void) {
    printf("%lld\n", (long long) fibo(35));
    return 0;
}
