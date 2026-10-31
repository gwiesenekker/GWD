#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

build_dir=$(mktemp -d /tmp/gwd8-movegen-test-XXXXXX)
compiler=${CC:-clang}
common=(-g -D_GNU_SOURCE -ffunction-sections -fdata-sections
        -I. -I"${EGTB_INCLUDE:-../endgame7}")
echo "Test artifacts: $build_dir"

if [[ ${1:-} == sanitize ]]; then
  # Legacy uses shifts by scalar square indices that can be out of range.
  # Keep ASan on it, but apply UBSan to the new generator and harness only.
  "$compiler" "${common[@]}" -O1 -fsanitize=address -c moves.c \
    -o "$build_dir/legacy.o"
  objcopy --keep-global-symbol=gen_moves_legacy "$build_dir/legacy.o"
  "$compiler" "${common[@]}" -O1 -fsanitize=address,undefined \
    -Dgen_moves_legacy=gen_moves_legacy_unused -c moves.c \
    -o "$build_dir/moves.o"
  "$compiler" "${common[@]}" -O1 -fsanitize=address,undefined \
    tests/test_movegen.c "$build_dir/legacy.o" "$build_dir/moves.o" \
    -Wl,--gc-sections -o "$build_dir/test"
  ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$build_dir/test"
else
  "$compiler" "${common[@]}" -O3 -march=native tests/test_movegen.c moves.c \
    -Wl,--gc-sections -o "$build_dir/test"
  "$build_dir/test"
  "$build_dir/test" legacy
  "$build_dir/test" bitboard
fi
