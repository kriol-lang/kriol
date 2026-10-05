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

# A '<source>.stdout' fixture is the exact output the program must write.
matches_stdout() {
    [ ! -f "$1.stdout" ] || cmp -s "$1.stdout" "$2"
}

# The value of the first '// <key>: <value>' line of a source file.
directive() {
    sed -n "s|^// $2: ||p" "$1" | head -n 1
}

# Runs a compiled program on its fixtures: '<runner> <program>' must exit 0 and
# write the expected output. An empty runner executes the program directly.
runs_cleanly() {
    local source="$1" runner="$2" program="$3" out
    out=$(mktemp /tmp/kriol_out_XXXX)
    timeout 5 $runner "$program" < "$(stdin_for "$source")" > "$out" 2>/dev/null && \
        matches_stdout "$source" "$out"
    local status=$?
    rm -f "$out"
    return $status
}

# Runs a compiled program that must stop with a runtime error: a non-zero exit
# status (or the '// exit:' one), the '// expect:' message on stderr, and only
# the expected output before it.
stops_as_expected() {
    local source="$1" runner="$2" program="$3" out err status=0 ok=1
    out=$(mktemp /tmp/kriol_out_XXXX); err=$(mktemp /tmp/kriol_err_XXXX)
    timeout 5 $runner "$program" < "$(stdin_for "$source")" > "$out" 2> "$err" || status=$?
    local expected_status expected_message
    expected_status=$(directive "$source" exit)
    expected_message=$(directive "$source" expect)
    if [ -n "$expected_status" ]; then
        [ "$status" -eq "$expected_status" ] || ok=0
    else
        [ "$status" -ne 0 ] && [ "$status" -ne 124 ] || ok=0
    fi
    [ -z "$expected_message" ] || grep -Fq -- "$expected_message" "$err" || ok=0
    matches_stdout "$source" "$out" || ok=0
    rm -f "$out" "$err"
    [ $ok -eq 1 ]
}

# Compiles one source for a target ('' is native) and checks it with a test
# function: runs_cleanly or stops_as_expected.
program_test() {
    local source="$1" target="$2" check="$3" label="$4"
    local program runner="" target_args=()
    printf "  %-44s" "$label"
    program=$(mktemp /tmp/kriol_XXXX)
    if [ -n "$target" ]; then
        target_args=(--target "$target")
        runner="node --no-warnings $ROOT/tests/wasm/run_wasi.mjs"
    fi
    if "$KRIOL" "$source" "${target_args[@]}" -o "$program" >/dev/null 2>&1 && \
       "$check" "$source" "$runner" "$program"; then
        echo " PASS"; pass=$((pass+1))
    else
        echo " FAIL"; record_failure "$label"
    fi
    rm -f "$program"
}

# ---- examples/*.kriol and tests/pass/*.kr: compile, run and compare output ----
shopt -s nullglob
PROGRAMS=("$ROOT"/examples/*.kriol "$ROOT"/tests/pass/*.kr)
PANICS=("$ROOT"/tests/panic/*.kr)
shopt -u nullglob

for f in "${PROGRAMS[@]}"; do
    program_test "$f" "" runs_cleanly "$f"
done

# ---- tests/panic/*.kr: programs that must stop with a runtime error ----------
for f in "${PANICS[@]}"; do
    program_test "$f" "" stops_as_expected "$f"
done

# ---- command-line source text ---------------------------------------------
printf "  %-44s" "inline source text"
tmpbin=$(mktemp /tmp/kriol_text_XXXX)
if "$KRIOL" --text 'fn inisiu() { mostran("Kualeh, Mundu!"); }' -o "$tmpbin" 2>/dev/null && \
   [ "$(timeout 5 "$tmpbin")" = "Kualeh, Mundu!" ]; then
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
UNHANDLED='fn f() int : Erru { lansa Erru{mensage = "boom"}; } fn inisiu() { int x = f(); mostran("never"); }'

printf "  %-44s" "unhandled error warns"
tmpbin=$(mktemp /tmp/kriol_warn_XXXX)
if "$KRIOL" --text "$UNHANDLED" -o "$tmpbin" 2>&1 | grep -Fq "aviso:" && [ -x "$tmpbin" ]; then
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

# ---- command-line errors ------------------------------------------------------
# rejects_with <label> <expected message> <kriol arguments...>
rejects_with() {
    local label="$1" expected="$2" output
    shift 2
    printf "  %-44s" "$label"
    if output=$("$KRIOL" "$@" 2>&1); then
        echo " FAIL (should have been rejected)"; record_failure "$label"
    elif ! printf '%s' "$output" | grep -Fq -- "$expected"; then
        echo " FAIL (expected: $expected)"; record_failure "$label"
    else
        echo " PASS"; pass=$((pass+1))
    fi
}

tmpdir=$(mktemp -d /tmp/kriol_cli_XXXX)
mkdir "$tmpdir/folder.kriol"
echo 'fn inisiu() { mostran("txt"); }' > "$tmpdir/program.txt"
rejects_with "cli: missing input" "falta o programa a compilar" -o /dev/null
rejects_with "cli: file and --text together" "um ficheiro ou --text" \
    "$tmpdir/program.txt" --text 'fn inisiu() {}'
rejects_with "cli: missing file" "não existe" "$tmpdir/missing.kriol"
rejects_with "cli: directory as input" "não é um ficheiro" "$tmpdir/folder.kriol"
rejects_with "cli: unknown extension" "não tem a extensão de um programa Kriol" "$tmpdir/program.txt"
rejects_with "cli: invalid optimization level" "o nível de otimização tem de ser" --text 'fn inisiu() {}' --opt-lvl 5
rejects_with "cli: unknown target" "não é suportado" --text 'fn inisiu() {}' --target sparc
rejects_with "cli: unknown option" "a opção '--sem-isto' não existe" --sem-isto
rejects_with "cli: option without a value" "falta o valor da opção '-o'" --text 'fn inisiu() {}' -o
rejects_with "cli: two files" "está a mais" "$tmpdir/a.kriol" "$tmpdir/b.kriol"

printf "  %-44s" "cli: --version"
if "$KRIOL" --version | grep -Eq "^Kriol v[0-9]+\.[0-9]+\.[0-9]+$"; then
    echo " PASS"; pass=$((pass+1))
else
    echo " FAIL"; record_failure "cli: --version"
fi

printf "  %-44s" "cli: --help"
if "$KRIOL" --help | grep -Fq "Utilização: kriol"; then
    echo " PASS"; pass=$((pass+1))
else
    echo " FAIL"; record_failure "cli: --help"
fi

printf "  %-44s" "cli: --ignore-extension"
if "$KRIOL" --ignore-extension "$tmpdir/program.txt" -o "$tmpdir/program" 2>/dev/null && \
   [ "$(timeout 5 "$tmpdir/program")" = "txt" ]; then
    echo " PASS"; pass=$((pass+1))
else
    echo " FAIL"; record_failure "cli: --ignore-extension"
fi

printf "  %-44s" "cli: --emit-ir writes the IR"
if "$KRIOL" --emit-ir --text 'fn inisiu() {}' -o "$tmpdir/program.ll" 2>/dev/null && \
   grep -Fq "define" "$tmpdir/program.ll"; then
    echo " PASS"; pass=$((pass+1))
else
    echo " FAIL"; record_failure "cli: --emit-ir writes the IR"
fi
rm -rf "$tmpdir"

# ---- tests/fail/*.kr -------------------------------------------------------
# A '// expect: <text>' line names a diagnostic the compiler must report.
if [ -d "$ROOT/tests/fail" ]; then
    for f in "$ROOT"/tests/fail/*.kr; do
        [ -f "$f" ] || continue
        printf "  %-44s" "$f"
        tmpbin=$(mktemp /tmp/kriol_fail_bin_XXXX)
        expected=$(sed -n 's|^// expect: ||p' "$f" | head -n 1)

        if output=$("$KRIOL" "$f" -o "$tmpbin" 2>&1); then
            echo " FAIL (should have been rejected)"
            record_failure "$f"
        elif [ -n "$expected" ] && ! printf '%s' "$output" | grep -Fq -- "$expected"; then
            echo " FAIL (expected: $expected)"
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

    # The same programs again as wasm modules, when Node can run them.
    if command -v node >/dev/null 2>&1; then
        for f in "${PROGRAMS[@]}"; do
            program_test "$f" wasm32-wasi runs_cleanly "wasm: ${f#"$ROOT"/}"
        done
        for f in "${PANICS[@]}"; do
            program_test "$f" wasm32-wasi stops_as_expected "wasm: ${f#"$ROOT"/}"
        done
    fi
fi

echo -e "\n  $pass/$((pass+fail)) passed\n"

if [ $fail -ne 0 ]; then
    echo -e "Failed tests:$failed_tests\n"
fi

[ $fail -eq 0 ]
