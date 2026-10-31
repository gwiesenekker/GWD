#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
compat_dir=${COMPAT_DIR:-../compat}
compat_dir=$(cd "$compat_dir" && pwd)
build_dir=$(mktemp -d /tmp/gwd8-compat-test-XXXXXX)
echo "Test artifacts: $build_dir"
flags=(-std=c11 -O1 -g -Wall -Wextra -Werror -pthread)
if [[ ${1:-} == sanitize ]]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
"${CC:-clang}" "${flags[@]}" -I"$compat_dir" \
  tests/test_compat.c "$compat_dir/compat.c" -o "$build_dir/test"
cd "$build_dir"
UBSAN_OPTIONS=halt_on_error=1 ./test
