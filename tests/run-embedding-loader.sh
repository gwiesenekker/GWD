#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d /tmp/gwd8-embedding-loader-XXXXXX)
echo "Test artifacts: $build_dir"
flags=(-std=c11 -D_GNU_SOURCE -O2 -g -march=native
       -ffunction-sections -fdata-sections -pthread
       -I. -I"${COMPAT_DIR:-../compat}" -I"${EGTB_INCLUDE:-../endgame7}")
if [[ ${1:-} == sanitize ]]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
"${CC:-clang}" "${flags[@]}" tests/test_embedding_loader.c cJSON.c \
  -Wl,--gc-sections -lm -o "$build_dir/test"
UBSAN_OPTIONS=halt_on_error=1 "$build_dir/test"
