#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d /tmp/gwd8-history-test-XXXXXX)
echo "Test artifacts: $build_dir"
"${CC:-clang}" -std=c11 -O1 -g -DDEBUG -D_GNU_SOURCE \
  -ffunction-sections -fdata-sections \
  -I. -I"${COMPAT_DIR:-../compat}" -I"${EGTB_INCLUDE:-../endgame7}" \
  -fsanitize=address,undefined tests/test_history.c my_random.c \
  "${COMPAT_DIR:-../compat}/compat.c" \
  -Wl,--gc-sections -lm -pthread -o "$build_dir/test"
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$build_dir/test"
"${CC:-clang}" -std=c11 -O3 -g -D_GNU_SOURCE -march=native \
  -ffunction-sections -fdata-sections \
  -I. -I"${COMPAT_DIR:-../compat}" -I"${EGTB_INCLUDE:-../endgame7}" \
  tests/test_history.c my_random.c "${COMPAT_DIR:-../compat}/compat.c" \
  -Wl,--gc-sections -lm -pthread -o "$build_dir/test-optimized"
"$build_dir/test-optimized"
