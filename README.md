# spsc_queue

A lock-free single-producer / single-consumer (SPSC) ring buffer in C11, using `<stdatomic.h>` with explicit, minimal memory ordering.

No mutexes, no semaphores, no `volatile` — synchronization is done entirely through acquire/release pairs on two indices.

## Design

- **Fixed capacity, power of 2** (`SPSC_CAP`). Slot index is computed with a mask (`idx & (SPSC_CAP - 1)`) instead of `%`.
- **Free-running indices.** `inPos` and `outPos` are never wrapped when stored; they are masked only when accessing `values[]`.
  - empty: `inPos - outPos == 0`
  - full: `inPos - outPos == SPSC_CAP`
  - No slot is wasted to distinguish full from empty. Unsigned wrap-around keeps the difference correct.
- **Single writer per index.**
  - `inPos` is written only by the producer (`spsc_push`).
  - `outPos` is written only by the consumer (`spsc_pop`).

## Memory ordering

Each thread reads its **own** index with `relaxed` (it is the only writer, so it always sees its last value) and the **other** thread's index with `acquire`, pairing with that thread's `release` store.

| Operation | Thread | Own index | Other index | Publish |
|---|---|---|---|---|
| `spsc_push` | producer | `inPos` — relaxed | `outPos` — acquire | `inPos` — release |
| `spsc_pop`  | consumer | `outPos` — relaxed | `inPos` — acquire | `outPos` — release |

- **Producer → consumer:** the slot write `values[i]` happens before the release on `inPos`. A consumer that observes the new `inPos` via acquire is guaranteed to see the data.
- **Consumer → producer:** the slot read happens before the release on `outPos`. A producer that observes the new `outPos` via acquire knows the slot is free to overwrite.

With `relaxed` on the publishing store, the CPU or compiler could make the index update visible before the slot write, and the consumer could read stale data.

## API

```c
void spsc_init(spsc_t *q);
bool spsc_push(spsc_t *q, uint32_t val);   /* false if full  */
bool spsc_pop(spsc_t *q, uint32_t *out);   /* false if empty */
bool spsc_is_empty(spsc_t *q);             /* call from consumer only */
bool spsc_is_full(spsc_t *q);              /* call from producer only */
```

`push` and `pop` are non-blocking. `spsc_is_empty` / `spsc_is_full` encode the calling thread in their memory orders, so they must only be called from the consumer and producer respectively.

## Build and test

```sh
make run    # -O2 build, runs the stress test
make tsan   # ThreadSanitizer build, runs the stress test
make clean
```

The test (`main.c`) starts one producer pushing `0..N-1` and one consumer checking that every value arrives in order. It prints `OK` or `FAIL (n errors)`.

### ThreadSanitizer on recent kernels

On kernels 6.5+ with older GCC/Clang, TSan may abort with `unexpected memory mapping`. Run with ASLR disabled for that process:

```sh
setarch $(uname -m) -R ./spsc_tsan
```

## Limitations / possible improvements

- Busy-wait on full/empty; no blocking or backoff.
- `inPos` and `outPos` share a cache line, so producer and consumer cause false sharing. Aligning each index to its own cache line (`alignas(64)`) would reduce coherence traffic.
- Each operation reads the other thread's index every time; caching it locally and refreshing only on apparent full/empty would cut cross-core loads further.
- The index is updated with `atomic_fetch_add`. Since each index has a single writer, a relaxed load followed by a release store is sufficient and avoids a locked RMW instruction on x86.
- Element type is fixed to `uint32_t`.
