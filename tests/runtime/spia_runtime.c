#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/wait.h>
#include <unistd.h>

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
void __kriol_check_div(int64_t lhs, int64_t rhs, int32_t signed_bits, int32_t line);
void __kriol_check_fdiv(double divisor, int32_t line);
void __kriol_check_bounds(int64_t index, int64_t size, int line);
void __kriol_assert(int cond, int line);
_Noreturn void __kriol_exit(int64_t code, int32_t is_signed, int32_t line);
_Noreturn void __kriol_panic(const char* message);
_Noreturn void __kriol_fail_at(const char* kind, const char* message, int32_t line);
_Noreturn void __kriol_raise(const char* message, const char* kind, int32_t origin_line, int32_t call_line);

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

static void kind_freed_after_the_throw_survives_the_jump(void) {
    char* volatile kind = malloc(8);
    CHECK(kind != NULL);
    memcpy(kind, "toma", 5);

    KriolSpiaFrame* frame = __kriol_spia_push();
    if (setjmp(*frame_jump(frame)) == 0) {
        __kriol_spia_throw(kind, "heap kind", 30);
    }
    memset(kind, 'x', 7);
    free(kind);
    CHECK(strcmp(__kriol_spia_kind(), "toma") == 0);
    churn_the_collector();
    CHECK(strcmp(__kriol_spia_kind(), "toma") == 0);
    CHECK(strcmp(__kriol_spia_message(), "heap kind") == 0);
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

static void expect_check_kind(void (*trigger)(void), const char* kind, const char* message, int32_t line) {
    volatile int caught = 0;
    KriolSpiaFrame* frame = __kriol_spia_push();
    if (setjmp(*frame_jump(frame)) == 0) {
        trigger();
    } else {
        caught = 1;
        CHECK(strcmp(__kriol_spia_kind(), kind) == 0);
        CHECK(strcmp(__kriol_spia_message(), message) == 0);
        CHECK(__kriol_spia_line() == line);
    }
    CHECK(caught);
    __kriol_spia_pop();
}

static void trigger_div(void) { __kriol_check_div(1, 0, 64, 3); }
static void trigger_overflow(void) { __kriol_check_div(INT64_MIN, -1, 64, 4); }
static void trigger_fdiv(void) { __kriol_check_fdiv(0.0, 5); }
static void trigger_bounds(void) { __kriol_check_bounds(5, 3, 4); }
static void trigger_assert(void) { __kriol_assert(0, 2); }
static void trigger_fail_at(void) { __kriol_fail_at("paniku", "boom", 9); }
static void trigger_raise(void) { __kriol_raise("falhou", "lansa", 11, 12); }
static void trigger_zeroed_erru(void) { __kriol_raise(NULL, NULL, 0, 6); }

static void runtime_checks_throw_with_their_kind(void) {
    expect_check_kind(trigger_div, "divizon", "divisão por zero", 3);
    expect_check_kind(trigger_overflow, "divizon", "o resultado da divisão não cabe no tipo do número inteiro", 4);
    expect_check_kind(trigger_fdiv, "divizon", "divisão por zero", 5);
    expect_check_kind(trigger_bounds, "indisi", "o índice 5 está fora do array, que vai de 0 a 2", 4);
    expect_check_kind(trigger_assert, "konfirma", "a condição de konfirma é falsa", 2);
    expect_check_kind(trigger_fail_at, "paniku", "boom", 9);
    expect_check_kind(trigger_raise, "lansa", "falhou", 11);
    expect_check_kind(trigger_zeroed_erru, "lansa", "", 0);
    CHECK(!__kriol_spia_active());
}

typedef struct {
    int status;
    char stderr_text[256];
} ChildResult;

static ChildResult run_child(void (*body)(const char*), const char* kind) {
    ChildResult result = {-1, ""};
    int fds[2];
    CHECK(pipe(fds) == 0);
    fflush(NULL);
    pid_t pid = fork();
    CHECK(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        dup2(fds[1], STDERR_FILENO);
        body(kind);
        _exit(99);
    }
    close(fds[1]);
    size_t used = 0;
    ssize_t got;
    while (used < sizeof result.stderr_text - 1 &&
           (got = read(fds[0], result.stderr_text + used, sizeof result.stderr_text - 1 - used)) > 0)
        used += (size_t)got;
    close(fds[0]);
    int status = 0;
    CHECK(waitpid(pid, &status, 0) == pid);
    result.status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return result;
}

static void child_exit_out_of_range(const char* kind) {
    (void)kind;
    KriolSpiaFrame* frame = __kriol_spia_push();
    if (setjmp(*frame_jump(frame)) == 0)
        __kriol_exit(300, 1, 1);
    _exit(99);
}

static void child_panic(const char* kind) {
    (void)kind;
    KriolSpiaFrame* frame = __kriol_spia_push();
    if (setjmp(*frame_jump(frame)) == 0)
        __kriol_panic("x");
    _exit(99);
}

static void child_raise_without_handler(const char* kind) {
    __kriol_raise("falhou", kind, 11, 12);
}

static void child_raise_zeroed_erru(const char* kind) {
    (void)kind;
    __kriol_raise(NULL, NULL, 0, 6);
}

static void expect_child(void (*body)(const char*), const char* kind, const char* expected_stderr) {
    ChildResult result = run_child(body, kind);
    CHECK(result.status == 1);
    CHECK(strcmp(result.stderr_text, expected_stderr) == 0);
}

static void uncatchable_errors_still_terminate(void) {
    expect_child(child_exit_out_of_range, "",
                 "kriol:1: erro de execução: o código de saída de 'sai' tem de estar entre 0 e 255, mas é 300\n");
    expect_child(child_panic, "", "kriol: erro de execução: x\n");
}

static void raise_without_a_handler_reports_by_kind(void) {
    const char* runtime_kinds[] = {"divizon", "indisi", "konfirma", "paniku"};
    for (size_t i = 0; i < sizeof runtime_kinds / sizeof *runtime_kinds; ++i)
        expect_child(child_raise_without_handler, runtime_kinds[i], "kriol:11: erro de execução: falhou\n");

    const char* value_kinds[] = {"lansa", "konverti", "toma"};
    for (size_t i = 0; i < sizeof value_kinds / sizeof *value_kinds; ++i)
        expect_child(child_raise_without_handler, value_kinds[i], "kriol:12: erro não tratado: falhou\n");

    expect_child(child_raise_zeroed_erru, "", "kriol:6: erro não tratado: \n");
}

int main(void) {
    __kriol_gc_init();
    catches_a_throw_from_a_deeper_call();
    nested_throw_reaches_the_inner_frame();
    message_built_on_the_stack_survives_the_jump();
    kind_freed_after_the_throw_survives_the_jump();
    repeated_throws_into_one_frame_keep_the_latest_error();
    runtime_checks_throw_with_their_kind();
    uncatchable_errors_still_terminate();
    raise_without_a_handler_reports_by_kind();
    return 0;
}
