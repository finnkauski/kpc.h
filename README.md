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
- `KPC_EVENT_CYCLES` — CPU clock cycles
- `KPC_EVENT_INSTRUCTIONS` — retired instructions
- `KPC_EVENT_BRANCHES` — branch instructions
- `KPC_EVENT_BRANCH_MISSES` — branch mispredictions
