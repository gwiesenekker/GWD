#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d /tmp/gwd8-thread-names-XXXXXX)
echo "Test artifacts: $build_dir"
flags=(-std=c11 -D_POSIX_C_SOURCE=200809L -DDEBUG -O1 -g -march=native
       -ffunction-sections -fdata-sections -pthread
       -I. -I"${COMPAT_DIR:-../compat}" -I"${EGTB_INCLUDE:-../endgame7}")
case ${1:-} in
  sanitize) flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer);;
  tsan) flags+=(-fsanitize=thread -fno-omit-frame-pointer);;
esac
"${CC:-clang}" "${flags[@]}" tests/test_thread_names.c \
  "${COMPAT_DIR:-../compat}/compat.c" -Wl,--gc-sections -lm -o "$build_dir/test"
UBSAN_OPTIONS=halt_on_error=1 TSAN_OPTIONS=halt_on_error=1 "$build_dir/test"
