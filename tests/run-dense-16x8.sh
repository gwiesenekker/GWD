#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d /tmp/gwd8-dense-16x8-XXXXXX)
echo "Test artifacts: $build_dir"
flags=(-std=c11 -D_GNU_SOURCE -O3 -g -march=native
       -ffunction-sections -fdata-sections -pthread
       -I. -I"${COMPAT_DIR:-../compat}" -I"${EGTB_INCLUDE:-../endgame7}")
if [[ ${1:-} == sanitize ]]; then
  flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
# Exercise both FMA policies without changing the production source setting.
sed 's/^#define USE_FMA$/#undef USE_FMA/' networks.c > "$build_dir/networks-no-fma.c"
for mode in avx2 avx2-no-fma scalar; do
  extra=()
  [[ $mode == scalar ]] || extra+=(-DUSE_AVX2_INTRINSICS)
  if [[ $mode == avx2-no-fma ]]; then
    extra+=(-ffp-contract=off "-DNETWORKS_TEST_SOURCE=\"$build_dir/networks-no-fma.c\"")
  fi
  "${CC:-clang}" "${flags[@]}" "${extra[@]}" tests/test_dense_16x8.c \
    -Wl,--gc-sections -lm -o "$build_dir/$mode"
  echo "$mode"
  UBSAN_OPTIONS=halt_on_error=1 "$build_dir/$mode"
done
