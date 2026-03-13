Wrapper for my own convenience over [ibireme's kpc_demo](https://gist.github.com/ibireme/173517c208c7dc333ba962c1f0d67d12) (public domain).

# kpc.h

Single-header C library for reading hardware performance counters on macOS via the private `kperf`/`kperfdata` frameworks. Supports Apple Silicon and Intel.

## Requirements

- macOS (Apple Silicon or Intel)
- Root privileges (`sudo`)

## Usage

```c
#define KPC_IMPLEMENTATION
#include "kpc.h"

int main(void) {
    if (!kpc_init()) return 1;

    kpc_u64 counter_map[KPC_MAX_COUNTERS] = {0};
    if (kpc_configure(counter_map)) return 1;

    kpc_u64 before[KPC_MAX_COUNTERS] = {0};
    kpc_u64 after[KPC_MAX_COUNTERS] = {0};

    kpc_read(before);
    // ... your code ...
    kpc_read(after);

    kpc_u64 cycles = after[counter_map[KPC_EVENT_CYCLES]] - before[counter_map[KPC_EVENT_CYCLES]];
    kpc_u64 instructions = after[counter_map[KPC_EVENT_INSTRUCTIONS]] - before[counter_map[KPC_EVENT_INSTRUCTIONS]];

    kpc_shutdown();
    return 0;
}
```

## API

### Demo 1 — profile current thread

- `kpc_init()` — load private frameworks via dlopen. Returns 0 on failure.
- `kpc_configure(counter_map)` — set up counters, fills `kpc_u64[KPC_MAX_COUNTERS]` map. Returns 0 on success.
- `kpc_read(buf)` — read current thread counters into a `kpc_u64[KPC_MAX_COUNTERS]`.
- `kpc_shutdown()` — disable counters.

### Demo 2 — profile external process

- `kpc_profile_pid(pid, time_s, sample_s)` — sample a process by PID for `time_s` seconds at `sample_s` intervals. Prints per-thread and aggregate results. Returns 0 on success.

## Events

Access counter indices via the `kpc_event_id` enum:

- `KPC_EVENT_CYCLES` — CPU clock cycles
- `KPC_EVENT_INSTRUCTIONS` — retired instructions
- `KPC_EVENT_BRANCHES` — branch instructions
- `KPC_EVENT_BRANCH_MISSES` — branch mispredictions
