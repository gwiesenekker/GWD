#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d /tmp/gwd8-headers-XXXXXX)
echo "Test artifacts: $build_dir"
flags=(-std=c11 -D_GNU_SOURCE -Werror -fsyntax-only
       -I. -I"${COMPAT_DIR:-../compat}" -I"${EGTB_INCLUDE:-../endgame7}")
headers=()
for header in *.h; do
  case $header in bstrlib.h|cJSON.h|xxhash.h) continue ;; esac
  headers+=("$header")
done
modes=(minimal debug avx2)
mpi_flags=()
if command -v mpicc >/dev/null 2>&1; then
  read -r -a mpi_flags <<< "$(mpicc --showme:compile)"
  modes+=(mpi)
else
  echo "MPI header checks skipped: mpicc not available"
fi
if [[ $(uname -m) == x86_64 ]]; then
  modes+=(profile)
fi
for mode in "${modes[@]}"; do
  extra=()
  case $mode in
    debug) extra+=(-DDEBUG -DUSE_SQLITE3) ;;
    avx2) extra+=(-DUSE_SQLITE3 -DUSE_AVX2_INTRINSICS -mavx2 -mfma) ;;
    mpi) extra+=(-DDEBUG -DUSE_SQLITE3 -DUSE_OPENMPI "${mpi_flags[@]}") ;;
    profile) extra+=(-DPROFILE) ;;
  esac
  for header in "${headers[@]}"; do
    # A header must provide its own dependencies and tolerate repeated inclusion.
    printf '#include "%s"\n#include "%s"\n' "$header" "$header" > "$build_dir/header.c"
    "${CC:-clang}" "${flags[@]}" "${extra[@]}" "$build_dir/header.c"
  done
  for order in forward reverse; do
    : > "$build_dir/order.c"
    if [[ $order == forward ]]; then
      for header in "${headers[@]}"; do
        printf '#include "%s"\n' "$header" >> "$build_dir/order.c"
      done
    else
      for ((iheader=${#headers[@]}-1; iheader>=0; iheader--)); do
        printf '#include "%s"\n' "${headers[iheader]}" >> "$build_dir/order.c"
      done
    fi
    "${CC:-clang}" "${flags[@]}" "${extra[@]}" "$build_dir/order.c"
  done
  echo "$mode: ${#headers[@]} standalone headers and both include orders PASS"
done
