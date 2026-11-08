# GWD changelog

## 8.1260

Released 8 November 2026, with Linux and Windows x86-64 binaries and the
shared 16x8 evaluation network. Changes below are relative to source version
8.1256; the binary release also includes the 8.1256 changes listed below.

- Added configurable ProbCut at eligible quiet, minimal-window search nodes.
  `prob_cut_depth` is the minimum depth for a probe; zero disables ProbCut.
  `prob_cut_percentage` selects the probe depth as a percentage of the current
  depth, and `prob_cut_margin` raises the probe threshold above beta.
  The packaged configuration uses 4, 50 and 20 respectively.
- Prevent immediate recursive ProbCut probes while allowing probes deeper in
  the move tree. Skip probes after reduced or extended moves, and when a
  sufficiently deep TT result already rules out the raised threshold.
  Successful probes return a fail-hard cutoff without storing the inferred
  result as a full-depth TT bound.
- Added probe and node counters for successful, failed and interrupted
  ProbCut searches. Attribute nested-probe nodes only once, to the innermost
  probe, so the counters show where probe work is spent.
- Corrected reduced-move tweak propagation in PV searches and internal
  iterative deepening. Avoid adding capture history twice when probing the
  same position for singularity or ProbCut.
- Fixed the total reported RAM usage for loaded WDL databases.
- Enable AVX2 evaluation intrinsics by default in the Linux Makefile, matching
  the Windows release build. Neural weights and arithmetic remain FP32.

The C/header/Makefile sources match SCU/8.1260. Release packages include
SHA-1 and SHA-256 checksums; endgame databases and opening books remain
separate downloads.

## 8.1256

7 November 2026. Source update; binaries have not been published for this version.

- Removed GWD's hard-coded seven-piece limit on WDL and DTM probes, allowing
  supported eight-piece positions to be handled by GWDEGTB.
- When a root DTM lookup succeeds but a required child-position DTM file is
  absent, report its filename, restore the board, and fall back to normal
  search instead of aborting. Discard the partial root DTM result.
  Other DTM failures are not treated as missing files.
- Create the master WDL probe cache once after loading the databases, rather
  than repeatedly inside the loading loop. Added per-database loading and
  probe-cache size diagnostics.
- Removed a duplicate DTM lookup while displaying the PV and updated DTM
  callers to the new optional missing-filename interface.

Building this version requires GWDEGTB with `gwdegtb_dtm_missing_name()` support.

## 8.1255 changes since 8.1236

Released 31 October 2026. Compared with 8.1236, released 1 May 2026.

GWD 8.1255 introduces padded bit-board move generation, GWDEGTB integration,
specialized neural-network kernels, revised selective search, and a C11-based
Windows build. Shared search caches and node reporting now use C11 atomics.

The release changes below describe the code in the
[8.1236 to 8.1255 comparison](https://github.com/gwiesenekker/GWD/compare/v8.1236...v8.1255).

### Move generation and search

- Replaced `gen_moves` with a generator using the existing padded bit-boards
  and diagonal shifts of 5 and 6. Retained `gen_moves_legacy` for comparison.
  The new generator filters potential capturing men with bit operations and
  follows the legacy piece/direction traversal. Capture handling retains
  captured pieces as blockers and allows return to the starting square
  (*rondslag*); different capture routes are not deduplicated.
- Reworked reduced and PV searches around separate move-list passes and
  reduced/full-depth re-searches. Removed the previous `nproven` and
  `DEPTH_MAX` proven-depth bookkeeping.
- Revised TT-based singular extensions: exclude the candidate move during the
  singular probe, extend the first move when appropriate, and propagate the
  extension into capture search. Fixed bugs in singular-extension handling.
- Added internal iterative deepening at PV nodes without a TT hit, from depth
  4 onward, using a half-depth search before probing the TT again.
- Changed root minimal-window searches to start below the current best score:
  `[best_score - SCORE(aspiration_margin), alpha + 1]`. A fail-high at this
  threshold is a reason to re-search, not yet evidence of a better move.
- Reduced misleading PV output: speculative bounds are printed selectively,
  while true-score results remain visible.
- Added and corrected `pv_equal` handling. When the best move and compared PV
  remain unchanged, the search can erase associated TT information and retry
  the depth instead of immediately deepening.
- Fixed initialization of newly occupied PV hash slots. The released PV
  representation remains a 64-entry, four-slot hash table; the experimental
  linear PV representation is not part of 8.1255.
- Updated move shuffling to accept a preferred first move, use multiply-high
  range mapping, and move expensive permutation validation under `DEBUG`.
  The released implementation still shuffles the full list, then places the
  preferred move first; it does not implement the experimental two-move limit.
- Prefetches TT entries through `compat_prefetch_read`.

Source areas: `moves.c`, `search.c`, `search.h`, `my_random.c`.

### Neural evaluation and network loading

- Reworked evaluation orientation to mirror the board for the network, with
  corresponding score-orientation handling.
- Expanded network support with logits-related changes, two-head output,
  pragma handling, additional pattern layouts, and material features including
  `nwhite_king20` and `nblack_king20`. Removed the older signals machinery and
  several unused feature definitions.
- Added compact pattern-state maps and further bit-based base-3 indexing
  implementations. `base3_index_v4` remains implemented in `networks.c` in
  this release; it was not moved inline into a header.
- Fixed the base64 matrix reader so either the row-major or transposed output
  can be requested independently. Shared allocations use the destination
  element size, and unwanted matrix orientations need not be allocated.
- Added AVX2 embedding-summation paths for 16-element and 64-element embeddings,
  selected after checking the corresponding 16x8 or 64x16 first-layer shape
  and all relevant embedding widths. These cover pattern and material branches.
- Added specialized 16x8 and 32x8 dense-layer kernels with load-time shape
  flags. The 64x16 embedding optimization is distinct from these dense kernels.
- Centralized AVX2 multiply-add selection in `MM256_MADD_PS`, supporting fused
  and separate multiply/add implementations. GELUV2 also has compile-time
  reciprocal-square-root and FMA branches.
- Use unaligned AVX2 loads for shared weight storage, avoiding assumptions
  about MPI-provided alignment. Locally aligned network state has compile-time
  layout checks.
- Changed search-score precision. Despite the historical name
  `SCORE_T_FLOAT`, the released `score_t` is a 32-bit integer scaled by 1000;
  displayed scores can therefore include three decimal places. Neural
  arithmetic still uses floating point.
- In 8.1255, `network2material_score` is an integer. In the scaled-score path,
  network output is rounded after multiplying by `SCORE(1)`, then multiplied
  by that integer conversion factor.

Tests at 8.1252 produced matching node counts on Linux, a Windows VM, and
Alienware Aurora. This result applies to the tested configuration, not a
guarantee of identical results for every compiler, network, or search setting.

Source areas: `networks.c`, `networks.h`, `patterns.c`, `patterns.h`,
`score.c`, `globals.h`.

### Endgame databases

- Replaced the large in-tree legacy endgame implementation with calls into
  the external endgame7/GWDEGTB library.
- Added GWDEGTB WDL loading and compressed-WDL support, including multithreaded
  loading and shared-memory storage for MPI configurations.
- Added a mutex around DTM probing to serialize access to the shared probe
  path during multithreaded search.
- Endgame database files are not bundled with the release and must be
  configured separately.

Source areas: `endgame.c`, `endgame.h`, `Makefile`, `CMakeLists.txt`.

### Threading and portability

- Added C11 atomic load/store wrappers for individual 64-bit words in the
  transposition table and score cache, using relaxed memory ordering and
  retaining checksum validation. Whole cache records are not atomic snapshots.
  Startup checks verify that these atomic words are lock-free.
- Changed TT integrity checking from the baseline's 32-bit scheme to the
  final 64-bit SplitMix64-style mixing scheme.
- Kept hot node counters thread-local to the search and published totals
  periodically through separate atomic counters. Worker-start acknowledgement
  prevents a previous search's final publication from contaminating a new one.
- Made DEBUG name-stack state and SQLite retry RNG state thread-local.
- Adopted C11 alignment and static layout assertions, with compatibility
  wrappers for nonstandard operations such as bit reversal, multiply-high,
  prefetch and compiler attributes.
- Updated socket types and operations for Windows, including socket-sized
  handles and compatible close calls. Added checks for failures from several
  thread, file, sleep and other compatibility operations.
- Corrected initialization and rank/size behavior when OpenMPI is disabled,
  supporting one local process with multiple search threads.
- Moved `compat.c` and `compat.h` out of the GWD repository into the shared
  external compat dependency used with endgame7.

Source areas: `search.c`, `score.c`, `my_threads.c`, `my_threads.h`,
`my_malloc.c`, `my_sqlite3.c`, `my_mpi.c`, `dxp.c`, `globals.h`.

### Protocols and maintenance

- Fixed Hub position handling so supplied moves are also applied to the
  search board. Added Hub MCTS shoot-outs with result and confidence-bound
  reporting, and updated abort/message handling.
- Fixed MCTS result handling, added timer/progress cleanup, fixed memory leaks,
  and reduced `update_db` memory use.
- Changed queue-full handling from immediate failure to waiting/retrying.
- Improved DXP time accounting with fractional seconds internally and rounded
  protocol output. Added receive-buffer bounds checking and socket cleanup.
- Added timer/progress destruction and expanded diagnostics; updated score
  formatting throughout protocol, book and diagnostic paths for scaled scores.
- Removed unused definitions and fields across the source. Many otherwise
  unchanged files also receive the updated SCU revision header.

### Builds and release contents

- Added a C11 CMake build, Windows instructions and `build.bat`, with external
  compat/endgame7 source paths and SQLite3/Zstd dependencies. Windows uses
  Clang/clang-cl and does not support OpenMPI in this build.
- Added `GWD_USE_AVX2` in CMake and `MAKE_AVX2` in the Makefile to select GWD
  evaluation intrinsics. On x86-64, disabling these evaluation intrinsics does
  not disable AVX2 in xxHash or necessarily in compiler-generated code.
- Added regression-test sources/scripts and Makefile targets for move generation,
  PV handling, dense kernels, compatibility, no-MPI operation, thread names and
  atomic caches, including sanitizer variants. Their inclusion is not a claim
  that every test was rerun for the published binaries.
- Added the TensorFlow embedding-training script to the published source tree,
  along with formatting/static-analysis configuration and an allowlist-based
  `.gitignore` that excludes local datasets, SCU snapshots and build products.
- Published separate Linux and Windows binary packages, a shared network
  download, and SHA-1/SHA-256 checksum lists. Endgame databases and books are
  separate; binary packaging defaults use no preloaded EGTBs.
