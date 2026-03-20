// kpc.h — single-header Apple Silicon / Intel performance counter API for macOS
// Based on ibireme's kpc_demo (public domain)
// https://gist.github.com/ibireme/173517c208c7dc333ba962c1f0d67d12
//
// Usage:
//   #define KPC_IMPLEMENTATION in exactly one .c file before including.
//
// API:
//   kpc_init()                  — load frameworks, returns 0 on failure
//   kpc_configure(map)          — set up counters, fills counter_map, returns 0 on success
//   kpc_read(buf)               — read current thread counters into buf
//   kpc_shutdown()              — disable counters
//   kpc_tick_frequency()        — kperf tick frequency
//
// Requires: sudo (root privileges)
//
// Options (define before including):
//   KPC_QUIET   — suppress info prints (errors still print)
//   KPC_DISABLE — stub out all functions (no sudo needed, counters return 0)

#ifndef KPC_H
#define KPC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t  kpc_u8;
typedef uint32_t kpc_u32;
typedef int32_t  kpc_i32;
typedef uint64_t kpc_u64;

// -------------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------------

#define KPC_CLASS_FIXED_MASK          (1u << 0)
#define KPC_CLASS_CONFIGURABLE_MASK   (1u << 1)
#define KPC_MAX_COUNTERS 32
#define KPC_EVENT_NAME_MAX 8

// -------------------------------------------------------------------------
// Types
// -------------------------------------------------------------------------

typedef kpc_u64 kpc_config_t;

typedef struct {
  const char *name;
  const char *description;
  const char *errata;
  const char *alias;
  const char *fallback;
  kpc_u32 mask;
  kpc_u8 number;
  kpc_u8 umask;
  kpc_u8 reserved;
  kpc_u8 is_fixed;
} kpep_event;

typedef struct {
  const char *name;
  const char *cpu_id;
  const char *marketing_name;
  void *plist_data;
  void *event_map;
  kpep_event *event_arr;
  kpep_event **fixed_event_arr;
  void *alias_map;
  size_t reserved_1;
  size_t reserved_2;
  size_t reserved_3;
  size_t event_count;
  size_t alias_count;
  size_t fixed_counter_count;
  size_t config_counter_count;
  size_t power_counter_count;
  kpc_u32 archtecture;
  kpc_u32 fixed_counter_bits;
  kpc_u32 config_counter_bits;
  kpc_u32 power_counter_bits;
} kpep_db;

typedef struct {
  kpep_db *db;
  kpep_event **ev_arr;
  size_t *ev_map;
  size_t *ev_idx;
  kpc_u32 *flags;
  kpc_u64 *kpc_periods;
  size_t event_count;
  size_t counter_count;
  kpc_u32 classes;
  kpc_u32 config_counter;
  kpc_u32 power_counter;
  kpc_u32 reserved;
} kpep_config;

// -------------------------------------------------------------------------
// Event aliases
// -------------------------------------------------------------------------

typedef struct {
  const char *alias;
  const char *names[KPC_EVENT_NAME_MAX];
} kpc_event_alias;

typedef enum {
  KPC_EVENT_CYCLES = 0,
  KPC_EVENT_INSTRUCTIONS,
  KPC_EVENT_BRANCHES,
  KPC_EVENT_BRANCH_MISSES,
  KPC_EVENT_COUNT,
} kpc_event_id;

static const kpc_event_alias kpc_events[KPC_EVENT_COUNT] = {
    [KPC_EVENT_CYCLES] = {"cycles",
                          {"FIXED_CYCLES", "CPU_CLK_UNHALTED.THREAD",
                           "CPU_CLK_UNHALTED.CORE"}},
    [KPC_EVENT_INSTRUCTIONS] = {"instructions",
                                {"FIXED_INSTRUCTIONS", "INST_RETIRED.ANY"}},
    [KPC_EVENT_BRANCHES] = {"branches",
                            {"INST_BRANCH", "BR_INST_RETIRED.ALL_BRANCHES",
                             "INST_RETIRED.ANY"}},
    [KPC_EVENT_BRANCH_MISSES] = {"branch-misses",
                                 {"BRANCH_MISPRED_NONSPEC", "BRANCH_MISPREDICT",
                                  "BR_MISP_RETIRED.ALL_BRANCHES",
                                  "BR_INST_RETIRED.MISPRED"}},
};

// -------------------------------------------------------------------------
// API
// -------------------------------------------------------------------------

#ifdef KPC_DISABLE
static inline int kpc_init(void) { return 1; }
static inline int kpc_configure(kpc_u64 *counter_map) { (void)counter_map; return 0; }
static inline void kpc_read(kpc_u64 *buf) { (void)buf; }
static inline void kpc_shutdown(void) {}
static inline kpc_u64 kpc_tick_frequency(void) { return 0; }
#else
int kpc_init(void);
int kpc_configure(kpc_u64 *counter_map);
void kpc_read(kpc_u64 *buf);
void kpc_shutdown(void);
kpc_u64 kpc_tick_frequency(void);
#endif

#endif // KPC_H

// =========================================================================
#if defined(KPC_IMPLEMENTATION) && !defined(KPC_DISABLE)

#include <dlfcn.h>
#include <stdio.h>

#ifdef KPC_QUIET
#define kpc__info(...) ((void)0)
#else
#define kpc__info(...) printf(__VA_ARGS__)
#endif
#define kpc__err(...) printf(__VA_ARGS__)

// -------------------------------------------------------------------------
// Function pointers
// -------------------------------------------------------------------------

// kperf
static int  (*kpc__set_counting)(kpc_u32 classes);
static int  (*kpc__set_thread_counting)(kpc_u32 classes);
static int  (*kpc__set_config)(kpc_u32 classes, kpc_config_t *config);
static int  (*kpc__get_thread_counters)(kpc_u32 tid, kpc_u32 buf_count, kpc_u64 *buf);
static int  (*kpc__force_all_ctrs_set)(int val);
static int  (*kpc__force_all_ctrs_get)(int *val_out);
static kpc_u64 (*kperf__tick_frequency)(void);

// kperfdata
static int  (*kpep__config_create)(kpep_db *db, kpep_config **cfg_ptr);
static int  (*kpep__config_add_event)(kpep_config *cfg, kpep_event **ev_ptr, kpc_u32 flag, kpc_u32 *err);
static int  (*kpep__config_force_counters)(kpep_config *cfg);
static int  (*kpep__config_kpc)(kpep_config *cfg, kpc_config_t *buf, size_t buf_size);
static int  (*kpep__config_kpc_count)(kpep_config *cfg, size_t *count_ptr);
static int  (*kpep__config_kpc_classes)(kpep_config *cfg, kpc_u32 *classes_ptr);
static int  (*kpep__config_kpc_map)(kpep_config *cfg, size_t *buf, size_t buf_size);
static int  (*kpep__db_create)(const char *name, kpep_db **db_ptr);
static int  (*kpep__db_event)(kpep_db *db, const char *name, kpep_event **ev_ptr);

// -------------------------------------------------------------------------
// Symbol loading
// -------------------------------------------------------------------------

typedef struct {
  const char *name;
  void **impl;
} kpc__lib_symbol;

#define KPC__NELEMS(x) (sizeof(x) / sizeof((x)[0]))

static const kpc__lib_symbol kpc__symbols_kperf[] = {
    {"kpc_set_counting",        (void **)&kpc__set_counting},
    {"kpc_set_thread_counting", (void **)&kpc__set_thread_counting},
    {"kpc_set_config",          (void **)&kpc__set_config},
    {"kpc_get_thread_counters", (void **)&kpc__get_thread_counters},
    {"kpc_force_all_ctrs_set",  (void **)&kpc__force_all_ctrs_set},
    {"kpc_force_all_ctrs_get",  (void **)&kpc__force_all_ctrs_get},
    {"kperf_tick_frequency",    (void **)&kperf__tick_frequency},
};

static const kpc__lib_symbol kpc__symbols_kperfdata[] = {
    {"kpep_config_create",         (void **)&kpep__config_create},
    {"kpep_config_add_event",      (void **)&kpep__config_add_event},
    {"kpep_config_force_counters", (void **)&kpep__config_force_counters},
    {"kpep_config_kpc",            (void **)&kpep__config_kpc},
    {"kpep_config_kpc_count",      (void **)&kpep__config_kpc_count},
    {"kpep_config_kpc_classes",    (void **)&kpep__config_kpc_classes},
    {"kpep_config_kpc_map",        (void **)&kpep__config_kpc_map},
    {"kpep_db_create",             (void **)&kpep__db_create},
    {"kpep_db_event",              (void **)&kpep__db_event},
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
  for (size_t i = 0; i < KPC__NELEMS(kpc__symbols_kperf); i++)
    *kpc__symbols_kperf[i].impl = NULL;
  for (size_t i = 0; i < KPC__NELEMS(kpc__symbols_kperfdata); i++)
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

  for (size_t i = 0; i < KPC__NELEMS(kpc__symbols_kperf); i++) {
    const kpc__lib_symbol *sym = &kpc__symbols_kperf[i];
    *sym->impl = dlsym(kpc__lib_handle_kperf, sym->name);
    if (!*sym->impl) {
      snprintf(kpc__lib_err_msg, sizeof(kpc__lib_err_msg),
               "Failed to load kperf function: %s", sym->name);
      kpc__return_err();
    }
  }

  for (size_t i = 0; i < KPC__NELEMS(kpc__symbols_kperfdata); i++) {
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
// Event lookup
// -------------------------------------------------------------------------

static kpep_event *kpc__find_event(kpep_db *db, const kpc_event_alias *alias) {
  for (kpc_u32 j = 0; j < KPC_EVENT_NAME_MAX; j++) {
    if (!alias->names[j]) break;
    kpep_event *ev = NULL;
    if (kpep__db_event(db, alias->names[j], &ev) == 0) return ev;
  }
  return NULL;
}

// -------------------------------------------------------------------------
// Public API
// -------------------------------------------------------------------------

int kpc_init(void) {
  if (!kpc__lib_init()) {
    kpc__err("kpc: %s\n", kpc__lib_err_msg);
    return 0;
  }
  return 1;
}

int kpc_configure(kpc_u64 *counter_map) {
  int force = 0;
  if (kpc__force_all_ctrs_get(&force)) {
    kpc__err("kpc: permission denied (needs root)\n");
    return 1;
  }

  kpep_db *db = NULL;
  if (kpep__db_create(NULL, &db)) {
    kpc__err("kpc: failed to load PMC database\n");
    return 1;
  }

  kpc__info("kpc: loaded db: %s (%s)\n", db->name, db->marketing_name);
  kpc__info("kpc: fixed counters: %zu, configurable counters: %zu\n",
         db->fixed_counter_count, db->config_counter_count);

  kpep_config *cfg = NULL;
  if (kpep__config_create(db, &cfg) || kpep__config_force_counters(cfg)) {
    kpc__err("kpc: failed to create config\n");
    return 1;
  }

  for (int i = 0; i < KPC_EVENT_COUNT; i++) {
    kpep_event *ev = kpc__find_event(db, &kpc_events[i]);
    if (!ev) {
      kpc__err("kpc: event not found: %s\n", kpc_events[i].alias);
      return 1;
    }
    if (kpep__config_add_event(cfg, &ev, 0, NULL)) {
      kpc__err("kpc: failed to add event: %s\n", kpc_events[i].alias);
      return 1;
    }
  }

  kpc_u32 classes = 0;
  size_t reg_count = 0;
  kpc_config_t regs[KPC_MAX_COUNTERS] = {0};
  size_t map[KPC_MAX_COUNTERS] = {0};

  if (kpep__config_kpc_classes(cfg, &classes) ||
      kpep__config_kpc_count(cfg, &reg_count) ||
      kpep__config_kpc_map(cfg, map, sizeof(map)) ||
      kpep__config_kpc(cfg, regs, sizeof(regs))) {
    kpc__err("kpc: failed to get config\n");
    return 1;
  }

  for (int i = 0; i < KPC_MAX_COUNTERS; i++)
    counter_map[i] = (kpc_u64)map[i];

  if (kpc__force_all_ctrs_set(1)) {
    kpc__err("kpc: failed to force counters\n");
    return 1;
  }

  if ((classes & KPC_CLASS_CONFIGURABLE_MASK) && reg_count) {
    if (kpc__set_config(classes, regs)) {
      kpc__err("kpc: failed to set config\n");
      return 1;
    }
  }

  if (kpc__set_counting(classes) || kpc__set_thread_counting(classes)) {
    kpc__err("kpc: failed to enable counting\n");
    return 1;
  }

  return 0;
}

void kpc_read(kpc_u64 *buf) {
  kpc__get_thread_counters(0, KPC_MAX_COUNTERS, buf);
}

void kpc_shutdown(void) {
  kpc__set_counting(0);
  kpc__set_thread_counting(0);
  kpc__force_all_ctrs_set(0);
}

kpc_u64 kpc_tick_frequency(void) {
  return kperf__tick_frequency();
}

#endif // KPC_IMPLEMENTATION
