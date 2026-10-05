#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static bool pertence(double cx, double cy) {
    double x = 0.0;
    double y = 0.0;
    for (int64_t i = 0; i < 200; i++) {
        double x2 = x * x;
        double y2 = y * y;
        if (x2 + y2 > 4.0) {
            return false;
        }
        y = 2.0 * x * y + cy;
        x = x2 - y2 + cx;
    }
    return true;
}

int main(void) {
    int64_t total = 0;
    for (int64_t py = 0; py < 600; py++) {
        for (int64_t px = 0; px < 800; px++) {
            double cx = -2.0 + 3.0 * px / 800;
            double cy = -1.2 + 2.4 * py / 600;
            if (pertence(cx, cy)) {
                total++;
            }
        }
    }
    printf("%lld\n", (long long) total);
    return 0;
}
