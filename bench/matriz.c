#include <stdint.h>
#include <stdio.h>

#define N 400

static double a[N][N], b[N][N], c[N][N];

int main(void) {
    int64_t n = N;
    for (int64_t i = 0; i < n; i++) {
        for (int64_t j = 0; j < n; j++) {
            a[i][j] = (double) (i + j) / n;
            b[i][j] = (double) (i + 2 * j) / n;
        }
    }
    for (int64_t i = 0; i < n; i++) {
        for (int64_t j = 0; j < n; j++) {
            double soma = 0.0;
            for (int64_t k = 0; k < n; k++) {
                soma += a[i][k] * b[k][j];
            }
            c[i][j] = soma;
        }
    }
    double traco = 0.0;
    for (int64_t i = 0; i < n; i++) {
        traco += c[i][i];
    }
    printf("%lld\n", (long long) traco);
    return 0;
}
