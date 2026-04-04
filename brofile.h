// brofiler: single-header profiler for macOS (Apple Silicon / Intel)
//
// Usage:
//   #define BROFILER_IMPLEMENTATION in exactly one .c file before including.
//
// API:
//   brofiler_start()              — set up performance counters, returns 0 on failure
//   brofiler_shutdown()          — disable counters and clean up
//   brofiler_snap(snap)          — capture a counter snapshot
//   brofiler_diff(a, b, out)     — compute b - a into out
//
// Requires: sudo (root privileges)
//
// Options (define before including):
//   BROFILER_DISABLE — stub out all functions (no sudo needed, counters return 0)
//   BROFILER_MAX_ANCHORS — number of timing points the profiler can support
//   BROFILER_INFO(fmt, ...) — override info printing (default: printf)
//   BROFILER_ERR(fmt, ...)  — override error printing (default: printf)

#ifndef BROFILER_H
#define BROFILER_H

#ifndef BROFILER_MAX_ANCHORS
#define BROFILER_MAX_ANCHORS 4096
#endif 

#include <inttypes.h>
#include <stdint.h>

// -------------------------------------------------------------------------
// Types
// -------------------------------------------------------------------------

typedef struct {
  uint64_t cycles;
  uint64_t instructions;
  uint64_t branches;
  uint64_t branch_misses;
} brofiler_snap;

typedef struct {
  const char* label;
  uint64_t hit_count;
  brofiler_snap data;
} brofiler_anchor;

typedef struct {
  brofiler_anchor anchors[BROFILER_MAX_ANCHORS];
  brofiler_snap start;
  brofiler_snap end;
} brofiler_state;


// -------------------------------------------------------------------------
// API
// -------------------------------------------------------------------------

#ifdef BROFILER_DISABLE
static inline int  brofiler_start(void) { return 1; }
static inline void brofiler_end(void) {}
static inline void brofiler_shutdown(void) {}
static inline void brofiler_snap_read(brofiler_snap *s) { *s = (brofiler_snap){0}; }
static inline void brofiler_diff(const brofiler_snap *a, const brofiler_snap *b, brofiler_snap *out) {
  out->cycles       = b->cycles       - a->cycles;
  out->instructions = b->instructions - a->instructions;
  out->branches     = b->branches     - a->branches;
  out->branch_misses = b->branch_misses - a->branch_misses;
}
#define BRO_TIME_START(name) ((void)0)
#define BRO_TIME_END(name)   ((void)0)
#else
int  brofiler_start(void);
void brofiler_end(void);
void brofiler_shutdown(void);
void brofiler_snap_read(brofiler_snap *s);
void brofiler_diff(const brofiler_snap *a, const brofiler_snap *b, brofiler_snap *out);

// -------------------------------------------------------------------------
// Timing macros
// -------------------------------------------------------------------------

static brofiler_state brofiler__state = {0};

#define BRO_TIME_START(name)                                                   \
    enum { brofiler__idx_##name = __COUNTER__ };                               \
    brofiler_snap brofiler__snap_##name;                                       \
    brofiler_snap_read(&brofiler__snap_##name)

#define BRO_TIME_END(name)                                                     \
    do {                                                                       \
        brofiler_snap brofiler__end_##name;                                    \
        brofiler_snap_read(&brofiler__end_##name);                             \
        brofiler_anchor *a = &brofiler__state.anchors[brofiler__idx_##name];   \
        a->label = #name;                                                      \
        a->hit_count++;                                                        \
        a->data.cycles        += brofiler__end_##name.cycles        - brofiler__snap_##name.cycles;        \
        a->data.instructions  += brofiler__end_##name.instructions  - brofiler__snap_##name.instructions;  \
        a->data.branches      += brofiler__end_##name.branches      - brofiler__snap_##name.branches;      \
        a->data.branch_misses += brofiler__end_##name.branch_misses - brofiler__snap_##name.branch_misses; \
    } while(0)
#endif

#endif // BROFILER_H

// =========================================================================
#if defined(BROFILER_IMPLEMENTATION) && !defined(BROFILER_DISABLE)

#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifndef BROFILER_VERBOSE
#define BROFILER_VERBOSE 1
#endif
#ifndef BROFILER_INFO
#if BROFILER_VERBOSE
#define BROFILER_INFO(...) printf(__VA_ARGS__)
#else
#define BROFILER_INFO(...) ((void)0)
#endif
#endif
#ifndef BROFILER_ERR
#define BROFILER_ERR(...) printf(__VA_ARGS__)
#endif

// =========================================================================
// KPC internals — Apple private performance counter API
// Based on ibireme's kpc_demo (public domain)
// https://gist.github.com/ibireme/173517c208c7dc333ba962c1f0d67d12
// =========================================================================

typedef uint8_t  kpc__u8;
typedef uint32_t kpc__u32;
typedef int32_t  kpc__i32;
typedef uint64_t kpc__u64;

#define KPC__CLASS_FIXED_MASK          (1u << 0)
#define KPC__CLASS_CONFIGURABLE_MASK   (1u << 1)
#define KPC__MAX_COUNTERS 32
#define KPC__EVENT_NAME_MAX 8

typedef kpc__u64 kpc__config_t;

typedef struct {
  const char *name;
  const char *description;
  const char *errata;
  const char *alias;
  const char *fallback;
  kpc__u32 mask;
  kpc__u8 number;
  kpc__u8 umask;
  kpc__u8 reserved;
  kpc__u8 is_fixed;
} kpc__event;

typedef struct {
  const char *name;
  const char *cpu_id;
  const char *marketing_name;
  void *plist_data;
  void *event_map;
  kpc__event *event_arr;
  kpc__event **fixed_event_arr;
  void *alias_map;
  size_t reserved_1;
  size_t reserved_2;
  size_t reserved_3;
  size_t event_count;
  size_t alias_count;
  size_t fixed_counter_count;
  size_t config_counter_count;
  size_t power_counter_count;
  kpc__u32 archtecture;
  kpc__u32 fixed_counter_bits;
  kpc__u32 config_counter_bits;
  kpc__u32 power_counter_bits;
} kpc__db;

typedef struct {
  kpc__db *db;
  kpc__event **ev_arr;
  size_t *ev_map;
  size_t *ev_idx;
  kpc__u32 *flags;
  kpc__u64 *kpc_periods;
  size_t event_count;
  size_t counter_count;
  kpc__u32 classes;
  kpc__u32 config_counter;
  kpc__u32 power_counter;
  kpc__u32 reserved;
} kpc__cfg;

// -------------------------------------------------------------------------
// Event aliases
// -------------------------------------------------------------------------

typedef struct {
  const char *alias;
  const char *names[KPC__EVENT_NAME_MAX];
} kpc__event_alias;

typedef enum {
  KPC__EVENT_CYCLES = 0,
  KPC__EVENT_INSTRUCTIONS,
  KPC__EVENT_BRANCHES,
  KPC__EVENT_BRANCH_MISSES,
  KPC__EVENT_COUNT,
} kpc__event_id;

static const kpc__event_alias kpc__events[KPC__EVENT_COUNT] = {
    [KPC__EVENT_CYCLES] = {"cycles",
                           {"FIXED_CYCLES", "CPU_CLK_UNHALTED.THREAD",
                            "CPU_CLK_UNHALTED.CORE"}},
    [KPC__EVENT_INSTRUCTIONS] = {"instructions",
                                 {"FIXED_INSTRUCTIONS", "INST_RETIRED.ANY"}},
    [KPC__EVENT_BRANCHES] = {"branches",
                             {"INST_BRANCH", "BR_INST_RETIRED.ALL_BRANCHES",
                              "INST_RETIRED.ANY"}},
    [KPC__EVENT_BRANCH_MISSES] = {"branch-misses",
                                  {"BRANCH_MISPRED_NONSPEC", "BRANCH_MISPREDICT",
                                   "BR_MISP_RETIRED.ALL_BRANCHES",
                                   "BR_INST_RETIRED.MISPRED"}},
};

// -------------------------------------------------------------------------
// Function pointers (loaded at runtime via dlopen)
// -------------------------------------------------------------------------

static int  (*kpc__set_counting)(kpc__u32 classes);
static int  (*kpc__set_thread_counting)(kpc__u32 classes);
static int  (*kpc__set_config)(kpc__u32 classes, kpc__config_t *config);
static int  (*kpc__get_thread_counters)(kpc__u32 tid, kpc__u32 buf_count, kpc__u64 *buf);
static int  (*kpc__force_all_ctrs_set)(int val);
static int  (*kpc__force_all_ctrs_get)(int *val_out);

static int  (*kpc__config_create)(kpc__db *db, kpc__cfg **cfg_ptr);
static int  (*kpc__config_add_event)(kpc__cfg *cfg, kpc__event **ev_ptr, kpc__u32 flag, kpc__u32 *err);
static int  (*kpc__config_force_counters)(kpc__cfg *cfg);
static int  (*kpc__config_kpc)(kpc__cfg *cfg, kpc__config_t *buf, size_t buf_size);
static int  (*kpc__config_kpc_count)(kpc__cfg *cfg, size_t *count_ptr);
static int  (*kpc__config_kpc_classes)(kpc__cfg *cfg, kpc__u32 *classes_ptr);
static int  (*kpc__config_kpc_map)(kpc__cfg *cfg, size_t *buf, size_t buf_size);
static int  (*kpc__db_create)(const char *name, kpc__db **db_ptr);
static int  (*kpc__db_event)(kpc__db *db, const char *name, kpc__event **ev_ptr);

// -------------------------------------------------------------------------
// Symbol loading
// -------------------------------------------------------------------------

typedef struct {
  const char *name;
  void **impl;
} kpc__lib_symbol;

#define BROFILER_NELEMS(x) (sizeof(x) / sizeof((x)[0]))

static const kpc__lib_symbol kpc__symbols_kperf[] = {
    {"kpc_set_counting",        (void **)&kpc__set_counting},
    {"kpc_set_thread_counting", (void **)&kpc__set_thread_counting},
    {"kpc_set_config",          (void **)&kpc__set_config},
    {"kpc_get_thread_counters", (void **)&kpc__get_thread_counters},
    {"kpc_force_all_ctrs_set",  (void **)&kpc__force_all_ctrs_set},
    {"kpc_force_all_ctrs_get",  (void **)&kpc__force_all_ctrs_get},
};

static const kpc__lib_symbol kpc__symbols_kperfdata[] = {
    {"kpep_config_create",         (void **)&kpc__config_create},
    {"kpep_config_add_event",      (void **)&kpc__config_add_event},
    {"kpep_config_force_counters", (void **)&kpc__config_force_counters},
    {"kpep_config_kpc",            (void **)&kpc__config_kpc},
    {"kpep_config_kpc_count",      (void **)&kpc__config_kpc_count},
    {"kpep_config_kpc_classes",    (void **)&kpc__config_kpc_classes},
    {"kpep_config_kpc_map",        (void **)&kpc__config_kpc_map},
    {"kpep_db_create",             (void **)&kpc__db_create},
    {"kpep_db_event",              (void **)&kpc__db_event},
};

#define KPC__LIB_PATH_KPERF     "/System/Library/PrivateFrameworks/kperf.framework/kperf"
#define KPC__LIB_PATH_KPERFDATA "/System/Library/PrivateFrameworks/kperfdata.framework/kperfdata"

static bool kpc__lib_inited = false;
static bool kpc__lib_has_err = false;
static char kpc__lib_err_msg[256];
static void *kpc__lib_handle_kperf = NULL;
static void *kpc__lib_handle_kperfdata = NULL;

static void kpc__lib_deinit(void) {
  kpc__lib_inited = false;
  kpc__lib_has_err = false;
  if (kpc__lib_handle_kperf) dlclose(kpc__lib_handle_kperf);
  if (kpc__lib_handle_kperfdata) dlclose(kpc__lib_handle_kperfdata);
  kpc__lib_handle_kperf = NULL;
  kpc__lib_handle_kperfdata = NULL;
  for (size_t i = 0; i < BROFILER_NELEMS(kpc__symbols_kperf); i++)
    *kpc__symbols_kperf[i].impl = NULL;
  for (size_t i = 0; i < BROFILER_NELEMS(kpc__symbols_kperfdata); i++)
    *kpc__symbols_kperfdata[i].impl = NULL;
}

static bool kpc__lib_init(void) {
#define kpc__return_err()                                                      \
  do {                                                                         \
    kpc__lib_deinit();                                                         \
    kpc__lib_inited = true;                                                    \
    kpc__lib_has_err = true;                                                   \
    return false;                                                              \
  } while (0)

  if (kpc__lib_inited) return !kpc__lib_has_err;

  kpc__lib_handle_kperf = dlopen(KPC__LIB_PATH_KPERF, RTLD_LAZY);
  if (!kpc__lib_handle_kperf) {
    snprintf(kpc__lib_err_msg, sizeof(kpc__lib_err_msg),
             "Failed to load kperf.framework: %s", dlerror());
    kpc__return_err();
  }

  kpc__lib_handle_kperfdata = dlopen(KPC__LIB_PATH_KPERFDATA, RTLD_LAZY);
  if (!kpc__lib_handle_kperfdata) {
    snprintf(kpc__lib_err_msg, sizeof(kpc__lib_err_msg),
             "Failed to load kperfdata.framework: %s", dlerror());
    kpc__return_err();
  }

  for (size_t i = 0; i < BROFILER_NELEMS(kpc__symbols_kperf); i++) {
    const kpc__lib_symbol *sym = &kpc__symbols_kperf[i];
    *sym->impl = dlsym(kpc__lib_handle_kperf, sym->name);
    if (!*sym->impl) {
      snprintf(kpc__lib_err_msg, sizeof(kpc__lib_err_msg),
               "Failed to load kperf function: %s", sym->name);
      kpc__return_err();
    }
  }

  for (size_t i = 0; i < BROFILER_NELEMS(kpc__symbols_kperfdata); i++) {
    const kpc__lib_symbol *sym = &kpc__symbols_kperfdata[i];
    *sym->impl = dlsym(kpc__lib_handle_kperfdata, sym->name);
    if (!*sym->impl) {
      snprintf(kpc__lib_err_msg, sizeof(kpc__lib_err_msg),
               "Failed to load kperfdata function: %s", sym->name);
      kpc__return_err();
    }
  }

  kpc__lib_inited = true;
  kpc__lib_has_err = false;
  return true;
#undef kpc__return_err
}

// -------------------------------------------------------------------------
// KPC event lookup
// -------------------------------------------------------------------------

static kpc__event *kpc__find_event(kpc__db *db, const kpc__event_alias *alias) {
  for (kpc__u32 j = 0; j < KPC__EVENT_NAME_MAX; j++) {
    if (!alias->names[j]) break;
    kpc__event *ev = NULL;
    if (kpc__db_event(db, alias->names[j], &ev) == 0) return ev;
  }
  return NULL;
}

// =========================================================================
// Brofiler state
// =========================================================================

static kpc__u64 brofiler__counter_map[KPC__MAX_COUNTERS];
static bool brofiler__started = false;

// =========================================================================
// Brofiler public API
// =========================================================================

int brofiler_start(void) {
  if (brofiler__started) return 1;

  if (!kpc__lib_init()) {
    BROFILER_ERR("brofiler: %s\n", kpc__lib_err_msg);
    return 0;
  }

  int force = 0;
  if (kpc__force_all_ctrs_get(&force)) {
    BROFILER_ERR("brofiler: permission denied (needs root)\n");
    return 0;
  }

  kpc__db *db = NULL;
  if (kpc__db_create(NULL, &db)) {
    BROFILER_ERR("brofiler: failed to load PMC database\n");
    return 0;
  }

  BROFILER_INFO("brofiler: loaded db: %s (%s)\n", db->name, db->marketing_name);
  BROFILER_INFO("brofiler: fixed counters: %zu, configurable counters: %zu\n",
                 db->fixed_counter_count, db->config_counter_count);

  kpc__cfg *cfg = NULL;
  if (kpc__config_create(db, &cfg) || kpc__config_force_counters(cfg)) {
    BROFILER_ERR("brofiler: failed to create config\n");
    return 0;
  }

  for (int i = 0; i < KPC__EVENT_COUNT; i++) {
    kpc__event *ev = kpc__find_event(db, &kpc__events[i]);
    if (!ev) {
      BROFILER_ERR("brofiler: event not found: %s\n", kpc__events[i].alias);
      return 0;
    }
    if (kpc__config_add_event(cfg, &ev, 0, NULL)) {
      BROFILER_ERR("brofiler: failed to add event: %s\n", kpc__events[i].alias);
      return 0;
    }
  }

  kpc__u32 classes = 0;
  size_t reg_count = 0;
  kpc__config_t regs[KPC__MAX_COUNTERS] = {0};
  size_t map[KPC__MAX_COUNTERS] = {0};

  if (kpc__config_kpc_classes(cfg, &classes) ||
      kpc__config_kpc_count(cfg, &reg_count) ||
      kpc__config_kpc_map(cfg, map, sizeof(map)) ||
      kpc__config_kpc(cfg, regs, sizeof(regs))) {
    BROFILER_ERR("brofiler: failed to get config\n");
    return 0;
  }

  for (int i = 0; i < KPC__MAX_COUNTERS; i++)
    brofiler__counter_map[i] = (kpc__u64)map[i];

  if (kpc__force_all_ctrs_set(1)) {
    BROFILER_ERR("brofiler: failed to force counters\n");
    return 0;
  }

  if ((classes & KPC__CLASS_CONFIGURABLE_MASK) && reg_count) {
    if (kpc__set_config(classes, regs)) {
      BROFILER_ERR("brofiler: failed to set config\n");
      return 0;
    }
  }

  if (kpc__set_counting(classes) || kpc__set_thread_counting(classes)) {
    BROFILER_ERR("brofiler: failed to enable counting\n");
    return 0;
  }

  brofiler__started = true;
  brofiler_snap_read(&brofiler__state.start);
  return 1;
}

void brofiler_shutdown(void) {
  kpc__set_counting(0);
  kpc__set_thread_counting(0);
  kpc__force_all_ctrs_set(0);
}

void brofiler_snap_read(brofiler_snap *s) {
  if (!kpc__get_thread_counters) {
    *s = (brofiler_snap){0};
    return;
  }
  kpc__u64 buf[KPC__MAX_COUNTERS] = {0};
  kpc__get_thread_counters(0, KPC__MAX_COUNTERS, buf);
  s->cycles       = buf[brofiler__counter_map[KPC__EVENT_CYCLES]];
  s->instructions = buf[brofiler__counter_map[KPC__EVENT_INSTRUCTIONS]];
  s->branches     = buf[brofiler__counter_map[KPC__EVENT_BRANCHES]];
  s->branch_misses = buf[brofiler__counter_map[KPC__EVENT_BRANCH_MISSES]];
}

void brofiler_diff(const brofiler_snap *a, const brofiler_snap *b, brofiler_snap *out) {
  out->cycles       = b->cycles       - a->cycles;
  out->instructions = b->instructions - a->instructions;
  out->branches     = b->branches     - a->branches;
  out->branch_misses = b->branch_misses - a->branch_misses;
}

void brofiler_end(void) {
  brofiler_snap_read(&brofiler__state.end);

  brofiler_snap total;
  brofiler_diff(&brofiler__state.start, &brofiler__state.end, &total);

  BROFILER_INFO("\n--- brofiler ---\n");
  BROFILER_INFO("%-20s %8s %12s %7s %12s %12s %12s %12s\n",
                "block", "hits", "cycles", "cy%", "cy/hit", "inst/hit", "br/hit", "miss/hit");

  uint64_t accounted_cycles = 0;
  for (int i = 0; i < BROFILER_MAX_ANCHORS; i++) {
    brofiler_anchor *a = &brofiler__state.anchors[i];
    if (!a->label) continue;

    uint64_t hits = a->hit_count ? a->hit_count : 1;
    double cy_pct = total.cycles ? 100.0 * (double)a->data.cycles / (double)total.cycles : 0.0;
    accounted_cycles += a->data.cycles;

    BROFILER_INFO("%-20s %8" PRIu64 " %12" PRIu64 " %6.2f%% %12" PRIu64 " %12" PRIu64 " %12" PRIu64 " %12" PRIu64 "\n",
                  a->label, a->hit_count, a->data.cycles, cy_pct,
                  a->data.cycles / hits, a->data.instructions / hits,
                  a->data.branches / hits, a->data.branch_misses / hits);
  }

  uint64_t unaccounted = total.cycles > accounted_cycles ? total.cycles - accounted_cycles : 0;
  double un_pct = total.cycles ? 100.0 * (double)unaccounted / (double)total.cycles : 0.0;
  BROFILER_INFO("%-20s %8s %12" PRIu64 " %6.2f%%\n", "other", "", unaccounted, un_pct);
  BROFILER_INFO("%-20s %8s %12" PRIu64 "\n", "total", "", total.cycles);
  BROFILER_INFO("\n");

  brofiler_shutdown();
}

#endif // BROFILER_IMPLEMENTATION
