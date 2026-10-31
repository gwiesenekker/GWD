#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d /tmp/gwd8-pv-test-XXXXXX)
echo "Test artifacts: $build_dir"
"${CC:-clang}" -O1 -g -DDEBUG -D_GNU_SOURCE -ffunction-sections -fdata-sections \
  -I. -I"${EGTB_INCLUDE:-../endgame7}" -fsanitize=address,undefined \
  tests/test_pv.c -Wl,--gc-sections -lm -o "$build_dir/test"
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$build_dir/test"
