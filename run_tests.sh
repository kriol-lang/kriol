#!/usr/bin/bash
# Usage: run_tests.sh <path-to-kriol-binary> <project-root>

set -e

KRIOL="$1"
ROOT="$2"

if [ -z "$KRIOL" ] || [ -z "$ROOT" ]; then
    echo "Usage: run_tests.sh <kriol-binary> <project-root>"
    exit 1
fi

echo -e "\n~~ Running tests ~~\n"
pass=0; fail=0
failed_tests=""

record_failure() {
    fail=$((fail+1))
    failed_tests="${failed_tests}\n  - $1"
}

stdin_for() {
    local source="$1"
    local fixture="${source}.stdin"

    if [ -f "$fixture" ]; then
        printf '%s' "$fixture"
    else
        printf '%s' /dev/null
    fi
}

# ---- examples/*.kr --------------------------------------------------------
for f in "$ROOT"/examples/*.kriol; do
    printf "  %-44s" "$f"
    tmpbin=$(mktemp /tmp/kriol_XXXX)
    stdin_file=$(stdin_for "$f")
    if "$KRIOL" "$f" -o "$tmpbin" 2>/dev/null && \
       timeout 5 "$tmpbin" < "$stdin_file" > /dev/null 2>&1; then
        echo " PASS"; pass=$((pass+1))
    else
        echo " FAIL"; record_failure "$f"
    fi
    rm -f "$tmpbin"
done

# ---- tests/pass/*.kr -------------------------------------------------------
if [ -d "$ROOT/tests/pass" ]; then
    for f in "$ROOT"/tests/pass/*.kr; do
        [ -f "$f" ] || continue
        printf "  %-44s" "$f"
        tmpbin=$(mktemp /tmp/kriol_XXXX)
        stdin_file=$(stdin_for "$f")
        if "$KRIOL" "$f" -o "$tmpbin" 2>/dev/null && \
           timeout 5 "$tmpbin" < "$stdin_file" > /dev/null 2>&1; then
            echo " PASS"; pass=$((pass+1))
        else
            echo " FAIL"; record_failure "$f"
        fi
        rm -f "$tmpbin"
    done
fi

# ---- command-line source text ---------------------------------------------
printf "  %-44s" "inline source text"
tmpbin=$(mktemp /tmp/kriol_text_XXXX)
if "$KRIOL" --text 'fn inisiu() { mostran("Kuale, Mundu!"); }' -o "$tmpbin" 2>/dev/null && \
   [ "$(timeout 5 "$tmpbin")" = "Kuale, Mundu!" ]; then
    echo " PASS"; pass=$((pass+1))
else
    echo " FAIL"; record_failure "inline source text"
fi
rm -f "$tmpbin"

# ---- optimization levels -----------------------------------------------------
for level in 0 3; do
    printf "  %-44s" "optimization level $level"
    tmpbin=$(mktemp /tmp/kriol_opt_XXXX)
    if "$KRIOL" --opt-lvl "$level" --text 'fn inisiu() { int[3] a = [1, 2, 3]; mostran(a[2] / a[0]); }' -o "$tmpbin" 2>/dev/null && \
       [ "$(timeout 5 "$tmpbin")" = "3" ]; then
        echo " PASS"; pass=$((pass+1))
    else
        echo " FAIL"; record_failure "optimization level $level"
    fi
    rm -f "$tmpbin"
done

# ---- unhandled errors: warning, --strict and the runtime stop -------------------
UNHANDLED='fn f() int : Erru { lansa Erru::{mensage: "boom"}; } fn inisiu() { int x = f(); mostran("never"); }'

printf "  %-44s" "unhandled error warns"
tmpbin=$(mktemp /tmp/kriol_warn_XXXX)
if "$KRIOL" --text "$UNHANDLED" -o "$tmpbin" 2>&1 | grep -Fq "warn:" && [ -x "$tmpbin" ]; then
    echo " PASS"; pass=$((pass+1))
else
    echo " FAIL"; record_failure "unhandled error warns"
fi

printf "  %-44s" "unhandled error stops the program"
unhandled_status=0
unhandled_out=$(timeout 5 "$tmpbin" 2>&1) || unhandled_status=$?
if [ "$unhandled_status" -ne 0 ] && echo "$unhandled_out" | grep -Fq "boom" && ! echo "$unhandled_out" | grep -Fq "never"; then
    echo " PASS"; pass=$((pass+1))
else
    echo " FAIL"; record_failure "unhandled error stops the program"
fi
rm -f "$tmpbin"

printf "  %-44s" "--strict rejects warnings"
if "$KRIOL" --strict --text "$UNHANDLED" -o /dev/null >/dev/null 2>&1; then
    echo " FAIL (should have been rejected)"; record_failure "--strict rejects warnings"
else
    echo " PASS"; pass=$((pass+1))
fi

printf "  %-44s" "--strict accepts clean programs"
tmpbin=$(mktemp /tmp/kriol_strict_XXXX)
if "$KRIOL" --strict "$ROOT/tests/pass/error_union_basic.kr" -o "$tmpbin" >/dev/null 2>&1; then
    echo " PASS"; pass=$((pass+1))
else
    echo " FAIL"; record_failure "--strict accepts clean programs"
fi
rm -f "$tmpbin"

# ---- intermediate files stay out of the output directory --------------------
printf "  %-44s" "output directory left untouched"
tmpdir=$(mktemp -d /tmp/kriol_out_XXXX)
echo precious > "$tmpdir/program.o"
if "$KRIOL" --text 'fn inisiu() { mostran("x"); }' -o "$tmpdir/program" 2>/dev/null && \
   [ "$(cat "$tmpdir/program.o" 2>/dev/null)" = "precious" ]; then
    echo " PASS"; pass=$((pass+1))
else
    echo " FAIL"; record_failure "output directory left untouched"
fi
rm -rf "$tmpdir"


# ---- tests/fail/*.kr -------------------------------------------------------
if [ -d "$ROOT/tests/fail" ]; then
    for f in "$ROOT"/tests/fail/*.kr; do
        [ -f "$f" ] || continue
        printf "  %-44s" "$f"
        tmpbin=$(mktemp /tmp/kriol_fail_bin_XXXX)

        if "$KRIOL" "$f" -o "$tmpbin" > /dev/null 2>&1; then
            echo " FAIL (should have been rejected)"
            record_failure "$f"
        else
            echo " PASS (rejected)"; pass=$((pass+1))
        fi
        rm -f "$tmpbin"
    done
fi

# ---- wasm32-wasi compile checks --------------------------------------------
if "$KRIOL" --target wasm32-wasi --text 'fn inisiu() {}' --emit-ir >/dev/null 2>&1; then
    printf "  %-44s" "wasm32-wasi hello_world"
    tmpwasm=$(mktemp /tmp/kriol_wasm_XXXX.wasm)
    if "$KRIOL" "$ROOT/examples/hello_world.kriol" --target wasm32-wasi -o "$tmpwasm" 2>/dev/null; then
        if command -v file >/dev/null 2>&1; then
            if file "$tmpwasm" | grep -Fq "WebAssembly"; then
                echo " PASS"; pass=$((pass+1))
            else
                echo " FAIL (not a WebAssembly module)"
                record_failure "wasm32-wasi hello_world"
            fi
        else
            echo " PASS"; pass=$((pass+1))
        fi
    else
        echo " FAIL"; record_failure "wasm32-wasi hello_world"
    fi
    rm -f "$tmpwasm"

    printf "  %-44s" "wasm32-wasi f-string runtime"
    tmpwasm=$(mktemp /tmp/kriol_wasm_fstr_gc_XXXX.wasm)
    if "$KRIOL" "$ROOT/tests/pass/mostra_interpolation.kr" --target wasm32-wasi -o "$tmpwasm" 2>/dev/null; then
        echo " PASS"; pass=$((pass+1))
    else
        echo " FAIL"; record_failure "wasm32-wasi f-string runtime"
    fi
    rm -f "$tmpwasm"
fi

echo -e "\n  $pass/$((pass+fail)) passed\n"

if [ $fail -ne 0 ]; then
    echo -e "Failed tests:$failed_tests\n"
fi

[ $fail -eq 0 ]
