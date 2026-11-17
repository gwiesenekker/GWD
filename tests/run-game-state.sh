#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d /tmp/gwd8-game-state-XXXXXX)
echo "Test artifacts: $build_dir"
"${CC:-clang}" -std=c11 -O1 -g -DDEBUG -D_GNU_SOURCE \
  -ffunction-sections -fdata-sections -fno-omit-frame-pointer \
  -I. -I"${COMPAT_DIR:-../compat}" -I"${EGTB_INCLUDE:-../endgame7}" \
  -fsanitize=address,undefined tests/test_game_state.c states.c bstrlib.c cJSON.c \
  "${COMPAT_DIR:-../compat}/compat.c" \
  -Wl,--gc-sections -lm -pthread -o "$build_dir/test"
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  "$build_dir/test" "$build_dir/game.json"
