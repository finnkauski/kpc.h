// Demo 1: profile a function in the current thread
// Build: cc -O2 demo_thread.c -o demo_thread
// Run:   sudo ./demo_thread

#define KPC_IMPLEMENTATION
#include "kpc.h"
#include <stdio.h>
#include <stdlib.h>

static void profile_func(void) {
  for (kpc_u32 i = 0; i < 100000; i++) {
    kpc_u32 r = arc4random();
    if (r % 2) arc4random();
  }
}

int main(void) {
  if (!kpc_init()) return 1;

  kpc_u64 counter_map[KPC_MAX_COUNTERS] = {0};
  if (kpc_configure(counter_map)) return 1;

  kpc_u64 before[KPC_MAX_COUNTERS] = {0};
  kpc_u64 after[KPC_MAX_COUNTERS] = {0};

  kpc_read(before);
  profile_func();
  kpc_read(after);

  printf("counters:\n");
  for (int i = 0; i < KPC_EVENT_COUNT; i++) {
    kpc_u64 idx = counter_map[i];
    kpc_u64 val = after[idx] - before[idx];
    printf("%14s: %llu\n", kpc_events[i].alias, (unsigned long long)val);
  }

  kpc_shutdown();
  return 0;
}
