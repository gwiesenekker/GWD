# Self-contained PV lists

A `pv_t` owns an `i16_t PV_moves[PV_MAX_MOVES + 1]` array.
Element zero is the next move; `INVALID` terminates the continuation.
There is no stored length, board key, inode, root offset, or external cursor.

`prepend_pv(parent, move, child)` constructs the parent line directly from
the child. It supports in-place prepending with overlapping storage.
`copy_pv` copies through the terminator only; `clear_pv` writes one sentinel.
Readers advance a local pointer through the list. Search frames and root
snapshots own independent buffers on their respective thread's stack.

The independent capacity is 1,024 moves plus the sentinel. At capacity,
prepending drops the last move and retains a terminated prefix. A 40-move
line copies 82 bytes including its sentinel, instead of the old fixed
2,048-byte hash table.

Search entry clears the output to prevent stale continuations after early
returns. Forced moves are stored explicitly. A transposition-table cutoff
records only its known move, without inventing a tail.

## Tests

```sh
make test-pv
make test-movegen
./a.out solve_problems std.fen
```

The PV unit tests run with DEBUG, ASan and UBSan and require no board or
search object. They cover empty and nonempty lists, independent snapshots,
live-only copying, in-place and separate-buffer prepend, self-copy, and
termination at capacity.

For a real four-thread search use `solve_problems_multi_threaded` with
plain FEN lines: that reader does not accept the `{0.0}` annotation in
`std.fen`.

The hash-table PV reference is not an exact node-count requirement for this
change. Its truncated PVs trigger extra “STUCK IN PV” re-searches. Explicit
lists distinguish those continuations, changing later cache contents,
node counts and scores. The move-generator reference log is left unchanged.
