# Move-generator regression tests

Run from the GWD8 directory:

```sh
make test-movegen
make test-movegen-sanitize
```

These standalone tests do not initialize MPI, load networks, or open EGTBs.
They use deterministic initial-position and random-material playouts, checking
both sides to move. They compare every move in order, including its origin,
destination, captured pieces, flags, and the board's `node_ncaptx` side effect.
The current corpus produces 2,203,977 comparisons and 1,546 return-to-origin
capture moves. Duplicate capture routes are intentionally retained, as in
`gen_moves_legacy`.

The sanitizer target applies ASan to both generators and UBSan to the new
generator and harness. UBSan is excluded from the legacy reference because its
scalar-square shifts can have out-of-range exponents at board edges.

The normal target also times each generator on the same deterministic corpus.
These are microbenchmarks, not whole-engine speed measurements. Run without
concurrent EGTB generation or other heavy workloads for meaningful comparisons.
Build products are retained in the printed temporary directory. The tests
require Clang and GNU-compatible `objcopy`; `CC` and `EGTB_INCLUDE` can override
the compiler and the default `../endgame7` include directory.

The integration regression is:

```sh
./a.out solve_problems std.fen
```

At ply 28, the new generator matches `tests/stdout0-std.txt` exactly:
`n=89599064/52891776/23829213`, move `35-30`, score `-11.268`.
The reference log's existing `PV ERROR` diagnostic is also present; this change
does not modify PV tracking.
