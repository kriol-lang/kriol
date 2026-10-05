#!/usr/bin/env bash
# Usage: scripts/coverage.sh <build-dir>
#
# Runs the API tests and run_tests.sh with a compiler configured with
# -DKRIOL_ENABLE_COVERAGE=ON, then reports the coverage of the compiler's own
# sources. LLVM_PROFDATA and LLVM_COV name the tools of the Clang that built it.

set -euo pipefail

BUILD=$(cd "$1" && pwd)
ROOT=$(cd "$(dirname "$0")/.." && pwd)
PROFDATA=${LLVM_PROFDATA:-llvm-profdata}
COV=${LLVM_COV:-llvm-cov}
OUT="$BUILD/coverage"

rm -rf "$OUT"
mkdir -p "$OUT"
export LLVM_PROFILE_FILE="$OUT/%p.profraw"

"$BUILD/kriol_api_compile_memory_test" > "$OUT/api.log"
"$BUILD/kriol_type_model_test" >> "$OUT/api.log"
"$ROOT/run_tests.sh" "$BUILD/kriol" "$ROOT" > "$OUT/tests.log"

"$PROFDATA" merge -sparse "$OUT"/*.profraw -o "$OUT/kriol.profdata"
rm -f "$OUT"/*.profraw

# Generated parser and scanner, system and third-party headers, and the tests
# themselves are not the compiler's sources.
COV_ARGS=(
    "$BUILD/kriol"
    -object "$BUILD/kriol_api_compile_memory_test"
    -object "$BUILD/kriol_type_model_test"
    -instr-profile="$OUT/kriol.profdata"
    -ignore-filename-regex="^/usr/|^$BUILD/|^$ROOT/tests/|^$ROOT/include/external/"
)
"$COV" report "${COV_ARGS[@]}" | tee "$OUT/report.txt"
"$COV" show "${COV_ARGS[@]}" -format=html -output-dir="$OUT/html" > /dev/null

echo
echo "HTML report: $OUT/html/index.html"
