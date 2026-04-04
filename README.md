Single-header C profiler for macOS using hardware performance counters via the private `kperf`/`kperfdata` frameworks. Apple Silicon and Intel. Based on [ibireme's kpc_demo](https://gist.github.com/ibireme/173517c208c7dc333ba962c1f0d67d12) (public domain).

# brofile.h

## Requirements

- macOS (Apple Silicon or Intel)
- Root privileges (`sudo`)

## Usage

```c
#define BROFILER_IMPLEMENTATION
#include "brofile.h"

int main(void) {
    if (!brofiler_start()) return 1;

    brofiler_snap before, after, diff;

    BRO_TIME_START(my_work);
    // ... your code ...
    BRO_TIME_END(my_work);

    brofiler_end();  // prints summary table and shuts down
    return 0;
}
```

### Manual snapshotting

```c
brofiler_snap before, after, diff;
brofiler_snap_read(&before);
// ... your code ...
brofiler_snap_read(&after);
brofiler_diff(&before, &after, &diff);
// diff.cycles, diff.instructions, diff.branches, diff.branch_misses
```

### Options

Define before including:

- `BROFILER_DISABLE` — stub out all functions (no sudo needed, counters return 0)
- `BROFILER_VERBOSE 0` — suppress info prints
- `BROFILER_INFO(fmt, ...)` / `BROFILER_ERR(fmt, ...)` — override printing
- `BROFILER_MAX_ANCHORS` — max timing points (default 4096)
