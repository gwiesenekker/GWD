#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d /tmp/gwd8-pv-test-XXXXXX)
echo "Test artifacts: $build_dir"
flags=(-std=c11 -g -D_GNU_SOURCE -ffunction-sections -fdata-sections
       -I. -I"${COMPAT_DIR:-../compat}" -I"${EGTB_INCLUDE:-../endgame7}")
for mode in optimized sanitize; do
  extra=(-O3 -march=native)
  if [[ $mode == sanitize ]]; then
    extra=(-O1 -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer)
  fi
  "${CC:-clang}" "${flags[@]}" "${extra[@]}" tests/test_pv.c \
    -Wl,--gc-sections -lm -o "$build_dir/$mode"
  echo "$mode"
  ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$build_dir/$mode"
done
