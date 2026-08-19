# オーケストレーターモード（ガイド付き調査）

[← セットアップとインストールは README](../README.md)

`/perf-orchestrator` はガイド付きモードです。hotspot から始め、各ターンの結果を読み取って「次に何を測るべきか」を提案します。`imgpipe` には[キャッシュミスのウォークスルー](cache-misses.md)で扱っていないボトルネックがあと 2 つあります。オーケストレーターを起動すると hotspot 実行後に次の計測の提案が出るので、その流れで試してみてください。2 つの内容を以下に記載します。

---

## その他のボトルネック（オーケストレーターモード用）

### Issue: pipeline stalled waiting on feeder (off-CPU)

#### Summary

The main loop blocks on `load_tile()` at the start of every frame. A background
feeder thread produces one token every 40 ms — the compute stages (`transform`,
`reduce`) finish far faster, so all worker threads sit idle waiting for the next
token. Off-CPU profiling shows a large fraction of wall time spent in a blocking
`read()` syscall, not in compute.

#### Cause

`imgpipe.c:33` — the feeder sleeps a fixed 40 ms between tokens:

```c
// feeder thread — imgpipe.c:28-39
static void *feeder(void *arg) {
    ...
    while (...) {
        struct timespec ts = {0, 40 * 1000 * 1000};   // 40 ms hard sleep
        nanosleep(&ts, NULL);
        write(load_pipe[1], &tok, 1);                  // one token per frame
    }
}
```

The main loop then blocks synchronously waiting for that token before it will
start any compute:

```c
// main loop — imgpipe.c:109-113
while (...) {
    if (!load_tile()) break;      // blocks here until feeder wakes up
    transform(src, dst);
    total += reduce(dst);
}
```

Because `transform` + `reduce` finish in roughly 100–200 ms on 4 threads, each
frame is dominated by the 40 ms stall on `load_tile()`, not by compute. The
pipeline is serialized: load → compute → load → compute, with no overlap.

#### Fix directions

1. **Overlap load with compute** — double-buffer: while compute runs on frame N,
   the feeder (or a dedicated I/O thread) pre-loads frame N+1 into a second
   buffer. The main loop picks up the ready buffer without waiting.

2. **Async / non-blocking load** — replace the blocking `read()` with a
   non-blocking check; if no token is ready, do other work (prefetch, prepare
   next tile) and poll again.

3. **Reduce feeder latency** — if the 40 ms sleep models real I/O (disk, network,
   sensor), reduce the source latency directly: larger batches, read-ahead, or a
   faster I/O path. This shrinks the stall but does not eliminate the
   serialization.

Option 1 (double-buffering) gives the most headroom: it hides the load latency
entirely behind compute, making the pipeline throughput-bound on compute rather
than latency-bound on I/O.

---

### Issue: unpredictable branch in reduce (branch misprediction)

#### Summary

`reduce._omp_fn.0` accounts for 99.87% of all branch mispredictions. IPC is 0.52
(normal is 2–4) and the branch-miss rate is 15.96% (normal is < 3%). The entire
misprediction cost concentrates on a single conditional at `imgpipe.c:65`.

#### Cause

The hot loop in `reduce` classifies each pixel by the value of its low byte:

```c
// imgpipe.c:63-74
for (size_t i = 0; i < (size_t)dim * dim; i++) {
    uint32_t v = img[i];
    if ((v & 0xff) >= 128) {          // ← imgpipe.c:65 — 100% of branch misses
        h = (h ^ v) * 0x100000001b3ULL;
        h ^= h >> 23;
        local += h & 0x3f;
    } else {
        h = (h + v) * 0x9e3779b97f4a7c15ULL;
        h ^= h << 17;
        local += (h >> 7) & 0x1f;
    }
}
```

Pixel values are generated from an xorshift PRNG and are uniformly distributed,
so `(v & 0xff) >= 128` is true roughly 50% of the time. The branch alternates
near-randomly between both arms — the CPU's branch predictor cannot learn any
pattern and mispredicts on nearly every other iteration, flushing the pipeline
each time.

Annotated assembly confirms the split:

```
51.23%  je     1998   ← jump taken (false arm)
48.77%  test   $0x80,%dl  // imgpipe.c:65
```

#### Fix directions

1. **Branchless rewrite** — compute both arms unconditionally and select with a
   conditional move (`cmov`), eliminating the jump entirely:

   ```c
   int hi = (v & 0xff) >= 128;
   uint64_t h0 = (h ^ v) * 0x100000001b3ULL; h0 ^= h0 >> 23;
   uint64_t h1 = (h + v) * 0x9e3779b97f4a7c15ULL; h1 ^= h1 << 17;
   h = hi ? h0 : h1;
   local += hi ? (h & 0x3f) : ((h >> 7) & 0x1f);
   ```

   Compilers typically emit `cmov` for the ternary form, removing the
   mispredicting `je`. Tradeoff: both multiplications execute every iteration.

2. **Sort / partition the input** — if the pixel data can be reordered so that
   low-byte < 128 pixels come first, the branch becomes predictable (long runs
   of the same outcome). Not always applicable to streaming image data.
