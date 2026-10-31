#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
            exit(1); \
        } \
    } while (0)

typedef struct KriolSpiaFrame KriolSpiaFrame;

void __kriol_gc_init(void);

KriolSpiaFrame* __kriol_spia_push(void);
void __kriol_spia_pop(void);
const char* __kriol_spia_message(void);
const char* __kriol_spia_kind(void);
int32_t __kriol_spia_line(void);
_Noreturn void __kriol_spia_throw(const char* kind, const char* message, int32_t line);
int __kriol_spia_active(void);

static jmp_buf* frame_jump(KriolSpiaFrame* frame) {
    return (jmp_buf*)frame;
}

__attribute__((noinline)) static void divide_by_zero(void) {
    __kriol_spia_throw("divizon", "x", 7);
}

__attribute__((noinline)) static void fail_with(const char* kind, int32_t line) {
    __kriol_spia_throw(kind, "inner", line);
}

static void catches_a_throw_from_a_deeper_call(void) {
    CHECK(!__kriol_spia_active());

    volatile int caught = 0;
    KriolSpiaFrame* frame = __kriol_spia_push();
    CHECK(__kriol_spia_active());

    if (setjmp(*frame_jump(frame)) == 0) {
        divide_by_zero();
    } else {
        caught = 1;
        CHECK(strcmp(__kriol_spia_kind(), "divizon") == 0);
        CHECK(strcmp(__kriol_spia_message(), "x") == 0);
        CHECK(__kriol_spia_line() == 7);
    }

    CHECK(caught);
    __kriol_spia_pop();
    CHECK(!__kriol_spia_active());
}

static void nested_throw_reaches_the_inner_frame(void) {
    volatile int outer_caught = 0;
    volatile int inner_caught = 0;

    KriolSpiaFrame* outer = __kriol_spia_push();
    if (setjmp(*frame_jump(outer)) == 0) {
        KriolSpiaFrame* inner = __kriol_spia_push();
        if (setjmp(*frame_jump(inner)) == 0) {
            fail_with("paniku", 11);
        } else {
            inner_caught = 1;
            CHECK(strcmp(__kriol_spia_kind(), "paniku") == 0);
            CHECK(__kriol_spia_line() == 11);
        }
        __kriol_spia_pop();

        CHECK(__kriol_spia_active());
        fail_with("lansa", 13);
    } else {
        outer_caught = 1;
        CHECK(strcmp(__kriol_spia_kind(), "lansa") == 0);
        CHECK(__kriol_spia_line() == 13);
    }
    __kriol_spia_pop();

    CHECK(inner_caught);
    CHECK(outer_caught);
    CHECK(!__kriol_spia_active());
}

int main(void) {
    __kriol_gc_init();
    catches_a_throw_from_a_deeper_call();
    nested_throw_reaches_the_inner_frame();
    return 0;
}
