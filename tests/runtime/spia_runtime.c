#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gc.h>

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
            exit(1); \
        } \
    } while (0)

typedef struct KriolSpiaFrame KriolSpiaFrame;

void __kriol_gc_init(void);
char* __kriol_format(const char* fmt, ...);

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

__attribute__((noinline)) static void fail_with_stack_message(int32_t line) {
    char message[64];
    snprintf(message, sizeof message, "stack message %d", (int)line);
    __kriol_spia_throw("indisi", message, line);
}

__attribute__((noinline)) static void clobber_stack(void) {
    volatile char scratch[4096];
    for (size_t i = 0; i < sizeof scratch; ++i)
        scratch[i] = 'z';
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

static void churn_the_collector(void) {
    GC_gcollect();
    for (int i = 0; i < 100000; ++i)
        CHECK(__kriol_format("churn %d", i) != NULL);
}

static void message_built_on_the_stack_survives_the_jump(void) {
    KriolSpiaFrame* frame = __kriol_spia_push();
    if (setjmp(*frame_jump(frame)) == 0) {
        fail_with_stack_message(21);
    }
    clobber_stack();
    churn_the_collector();
    CHECK(strcmp(__kriol_spia_message(), "stack message 21") == 0);
    CHECK(__kriol_spia_line() == 21);
    __kriol_spia_pop();
}

static void repeated_throws_into_one_frame_keep_the_latest_error(void) {
    enum { throw_count = 1000 };
    volatile int throws = 0;

    KriolSpiaFrame* frame = __kriol_spia_push();
    setjmp(*frame_jump(frame));
    if (throws < throw_count) {
        ++throws;
        fail_with_stack_message(throws);
    }

    clobber_stack();
    CHECK(throws == throw_count);
    CHECK(strcmp(__kriol_spia_message(), "stack message 1000") == 0);
    CHECK(__kriol_spia_line() == throw_count);
    __kriol_spia_pop();
    CHECK(!__kriol_spia_active());
}

int main(void) {
    __kriol_gc_init();
    catches_a_throw_from_a_deeper_call();
    nested_throw_reaches_the_inner_frame();
    message_built_on_the_stack_survives_the_jump();
    repeated_throws_into_one_frame_keep_the_latest_error();
    return 0;
}
