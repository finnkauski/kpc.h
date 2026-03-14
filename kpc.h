// kpc.h — single-header Apple Silicon / Intel performance counter API for macOS
// Based on ibireme's kpc_demo (public domain)
// https://gist.github.com/ibireme/173517c208c7dc333ba962c1f0d67d12
//
// Usage:
//   #define KPC_IMPLEMENTATION in exactly one .c file before including.
//
// Demo 1 — profile a function in the current thread:
//   kpc_init()           — load frameworks, returns 0 on failure
//   kpc_configure(map)   — set up counters, fills counter_map, returns 0 on success
//   kpc_read(buf)        — read current thread counters into buf
//   kpc_shutdown()       — disable counters
//
// Demo 2 — profile an external process by PID:
//   kpc_profile_pid(pid, time_s, sample_s)
//
// Requires: sudo (root privileges)
//
// Options (define before including):
//   KPC_QUIET — suppress info prints (errors still print)

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

#define KPC_CLASS_FIXED          0
#define KPC_CLASS_CONFIGURABLE   1
#define KPC_CLASS_POWER          2
#define KPC_CLASS_RAWPMU         3

#define KPC_CLASS_FIXED_MASK          (1u << KPC_CLASS_FIXED)
#define KPC_CLASS_CONFIGURABLE_MASK   (1u << KPC_CLASS_CONFIGURABLE)
#define KPC_CLASS_POWER_MASK          (1u << KPC_CLASS_POWER)
#define KPC_CLASS_RAWPMU_MASK         (1u << KPC_CLASS_RAWPMU)

#define KPC_PMU_ERROR     0
#define KPC_PMU_INTEL_V3  1
#define KPC_PMU_ARM_APPLE 2
#define KPC_PMU_INTEL_V2  3
#define KPC_PMU_ARM_V2    4

#define KPC_MAX_COUNTERS 32

#define KPERF_SAMPLER_TH_INFO        (1U << 0)
#define KPERF_SAMPLER_TH_SNAPSHOT    (1U << 1)
#define KPERF_SAMPLER_KSTACK         (1U << 2)
#define KPERF_SAMPLER_USTACK         (1U << 3)
#define KPERF_SAMPLER_PMC_THREAD     (1U << 4)
#define KPERF_SAMPLER_PMC_CPU        (1U << 5)
#define KPERF_SAMPLER_PMC_CONFIG     (1U << 6)
#define KPERF_SAMPLER_MEMINFO        (1U << 7)
#define KPERF_SAMPLER_TH_SCHEDULING  (1U << 8)
#define KPERF_SAMPLER_TH_DISPATCH    (1U << 9)
#define KPERF_SAMPLER_TK_SNAPSHOT    (1U << 10)
#define KPERF_SAMPLER_SYS_MEM        (1U << 11)
#define KPERF_SAMPLER_TH_INSCYC      (1U << 12)
#define KPERF_SAMPLER_TK_INFO        (1U << 13)

#define KPERF_ACTION_MAX 32
#define KPERF_TIMER_MAX  8

#define KPC_EVENT_NAME_MAX 8

#define KPEP_ARCH_I386   0
#define KPEP_ARCH_X86_64 1
#define KPEP_ARCH_ARM    2
#define KPEP_ARCH_ARM64  3

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

typedef enum {
  KPEP_CONFIG_ERROR_NONE = 0,
  KPEP_CONFIG_ERROR_INVALID_ARGUMENT,
  KPEP_CONFIG_ERROR_OUT_OF_MEMORY,
  KPEP_CONFIG_ERROR_IO,
  KPEP_CONFIG_ERROR_BUFFER_TOO_SMALL,
  KPEP_CONFIG_ERROR_CUR_SYSTEM_UNKNOWN,
  KPEP_CONFIG_ERROR_DB_PATH_INVALID,
  KPEP_CONFIG_ERROR_DB_NOT_FOUND,
  KPEP_CONFIG_ERROR_DB_ARCH_UNSUPPORTED,
  KPEP_CONFIG_ERROR_DB_VERSION_UNSUPPORTED,
  KPEP_CONFIG_ERROR_DB_CORRUPT,
  KPEP_CONFIG_ERROR_EVENT_NOT_FOUND,
  KPEP_CONFIG_ERROR_CONFLICTING_EVENTS,
  KPEP_CONFIG_ERROR_COUNTERS_NOT_FORCED,
  KPEP_CONFIG_ERROR_EVENT_UNAVAILABLE,
  KPEP_CONFIG_ERROR_ERRNO,
  KPEP_CONFIG_ERROR_MAX,
} kpep_config_error_code;

// kdebug types
typedef kpc_u64 kd_buf_argtype;

typedef struct {
  kpc_u64 timestamp;
  kd_buf_argtype arg1;
  kd_buf_argtype arg2;
  kd_buf_argtype arg3;
  kd_buf_argtype arg4;
  kd_buf_argtype arg5;
  kpc_u32 debugid;
#if defined(__LP64__) || defined(__arm64__)
  kpc_u32 cpuid;
  kd_buf_argtype unused;
#endif
} kd_buf;

typedef struct {
  unsigned int type;
  unsigned int value1;
  unsigned int value2;
  unsigned int value3;
  unsigned int value4;
} kd_regtype;

typedef struct {
  int nkdbufs;
  int nolog;
  unsigned int flags;
  int nkdthreads;
  int bufid;
} kbufinfo_t;

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

int kpc_init(void);
int kpc_configure(kpc_u64 *counter_map);
void kpc_read(kpc_u64 *buf);
void kpc_shutdown(void);
kpc_u64 kpc_tick_frequency(void);
int kpc_profile_pid(kpc_i32 pid, double profile_time_s, double sample_period_s);
const char *kpep_config_error_desc(int code);

#endif // KPC_H

// =========================================================================
#ifdef KPC_IMPLEMENTATION

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef KPC_QUIET
#define kpc__info(...) ((void)0)
#else
#define kpc__info(...) printf(__VA_ARGS__)
#endif
#define kpc__err(...) printf(__VA_ARGS__)
#include <string.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <sys/kdebug.h>
#include <sys/time.h>
#include <mach/mach_time.h>

// -------------------------------------------------------------------------
// Error descriptions
// -------------------------------------------------------------------------

static const char *kpep_config_error_names[KPEP_CONFIG_ERROR_MAX] = {
    "none",
    "invalid argument",
    "out of memory",
    "I/O",
    "buffer too small",
    "current system unknown",
    "database path invalid",
    "database not found",
    "database architecture unsupported",
    "database version unsupported",
    "database corrupt",
    "event not found",
    "conflicting events",
    "all counters must be forced",
    "event unavailable",
    "check errno",
};

const char *kpep_config_error_desc(int code) {
  if (0 <= code && code < KPEP_CONFIG_ERROR_MAX)
    return kpep_config_error_names[code];
  return "unknown error";
}

// -------------------------------------------------------------------------
// kperf function pointers
// -------------------------------------------------------------------------

static int  (*kpc__cpu_string)(char *buf, size_t buf_size);
static kpc_u32 (*kpc__pmu_version)(void);
static kpc_u32 (*kpc__get_counting)(void);
static int  (*kpc__set_counting)(kpc_u32 classes);
static kpc_u32 (*kpc__get_thread_counting)(void);
static int  (*kpc__set_thread_counting)(kpc_u32 classes);
static kpc_u32 (*kpc__get_config_count)(kpc_u32 classes);
static int  (*kpc__get_config)(kpc_u32 classes, kpc_config_t *config);
static int  (*kpc__set_config)(kpc_u32 classes, kpc_config_t *config);
static kpc_u32 (*kpc__get_counter_count)(kpc_u32 classes);
static int  (*kpc__get_cpu_counters)(bool all_cpus, kpc_u32 classes, int *curcpu, kpc_u64 *buf);
static int  (*kpc__get_thread_counters)(kpc_u32 tid, kpc_u32 buf_count, kpc_u64 *buf);
static int  (*kpc__force_all_ctrs_set)(int val);
static int  (*kpc__force_all_ctrs_get)(int *val_out);

static int  (*kperf__action_count_set)(kpc_u32 count);
static int  (*kperf__action_count_get)(kpc_u32 *count);
static int  (*kperf__action_samplers_set)(kpc_u32 actionid, kpc_u32 sample);
static int  (*kperf__action_samplers_get)(kpc_u32 actionid, kpc_u32 *sample);
static int  (*kperf__action_filter_set_by_task)(kpc_u32 actionid, kpc_i32 port);
static int  (*kperf__action_filter_set_by_pid)(kpc_u32 actionid, kpc_i32 pid);
static int  (*kperf__timer_count_set)(kpc_u32 count);
static int  (*kperf__timer_count_get)(kpc_u32 *count);
static int  (*kperf__timer_period_set)(kpc_u32 actionid, kpc_u64 tick);
static int  (*kperf__timer_period_get)(kpc_u32 actionid, kpc_u64 *tick);
static int  (*kperf__timer_action_set)(kpc_u32 actionid, kpc_u32 timerid);
static int  (*kperf__timer_action_get)(kpc_u32 actionid, kpc_u32 *timerid);
static int  (*kperf__timer_pet_set)(kpc_u32 timerid);
static int  (*kperf__timer_pet_get)(kpc_u32 *timerid);
static int  (*kperf__sample_set)(kpc_u32 enabled);
static int  (*kperf__sample_get)(kpc_u32 *enabled);
static int  (*kperf__reset)(void);
static kpc_u64 (*kperf__ns_to_ticks)(kpc_u64 ns);
static kpc_u64 (*kperf__ticks_to_ns)(kpc_u64 ticks);
static kpc_u64 (*kperf__tick_frequency)(void);

// kperfdata function pointers
static int  (*kpep__config_create)(kpep_db *db, kpep_config **cfg_ptr);
static void (*kpep__config_free)(kpep_config *cfg);
static int  (*kpep__config_add_event)(kpep_config *cfg, kpep_event **ev_ptr, kpc_u32 flag, kpc_u32 *err);
static int  (*kpep__config_remove_event)(kpep_config *cfg, size_t idx);
static int  (*kpep__config_force_counters)(kpep_config *cfg);
static int  (*kpep__config_events_count)(kpep_config *cfg, size_t *count_ptr);
static int  (*kpep__config_events)(kpep_config *cfg, kpep_event **buf, size_t buf_size);
static int  (*kpep__config_kpc)(kpep_config *cfg, kpc_config_t *buf, size_t buf_size);
static int  (*kpep__config_kpc_count)(kpep_config *cfg, size_t *count_ptr);
static int  (*kpep__config_kpc_classes)(kpep_config *cfg, kpc_u32 *classes_ptr);
static int  (*kpep__config_kpc_map)(kpep_config *cfg, size_t *buf, size_t buf_size);
static int  (*kpep__db_create)(const char *name, kpep_db **db_ptr);
static void (*kpep__db_free)(kpep_db *db);
static int  (*kpep__db_name)(kpep_db *db, const char **name);
static int  (*kpep__db_aliases_count)(kpep_db *db, size_t *count);
static int  (*kpep__db_aliases)(kpep_db *db, const char **buf, size_t buf_size);
static int  (*kpep__db_counters_count)(kpep_db *db, kpc_u8 classes, size_t *count);
static int  (*kpep__db_events_count)(kpep_db *db, size_t *count);
static int  (*kpep__db_events)(kpep_db *db, kpep_event **buf, size_t buf_size);
static int  (*kpep__db_event)(kpep_db *db, const char *name, kpep_event **ev_ptr);
static int  (*kpep__event_name)(kpep_event *ev, const char **name_ptr);
static int  (*kpep__event_alias)(kpep_event *ev, const char **alias_ptr);
static int  (*kpep__event_description)(kpep_event *ev, const char **str_ptr);

// -------------------------------------------------------------------------
// Lightweight PET (not in kperf.framework, accessed via sysctl)
// -------------------------------------------------------------------------

static int kperf_lightweight_pet_get(kpc_u32 *enabled) {
  if (!enabled) return -1;
  size_t size = 4;
  return sysctlbyname("kperf.lightweight_pet", enabled, &size, NULL, 0);
}

static int kperf_lightweight_pet_set(kpc_u32 enabled) {
  return sysctlbyname("kperf.lightweight_pet", NULL, NULL, &enabled, 4);
}

// -------------------------------------------------------------------------
// kdebug utilities
// -------------------------------------------------------------------------

#define KDBG_CLASSTYPE    0x10000
#define KDBG_SUBCLSTYPE   0x20000
#define KDBG_RANGETYPE    0x40000
#define KDBG_TYPENONE     0x80000
#define KDBG_CKTYPES      0xF0000
#define KDBG_VALCHECK     0x00200000U

static int kdebug_reset(void) {
  int mib[3] = {CTL_KERN, KERN_KDEBUG, KERN_KDREMOVE};
  return sysctl(mib, 3, NULL, NULL, NULL, 0);
}

static int kdebug_reinit(void) {
  int mib[3] = {CTL_KERN, KERN_KDEBUG, KERN_KDSETUP};
  return sysctl(mib, 3, NULL, NULL, NULL, 0);
}

static int kdebug_setreg(kd_regtype *kdr) {
  int mib[3] = {CTL_KERN, KERN_KDEBUG, KERN_KDSETREG};
  size_t size = sizeof(kd_regtype);
  return sysctl(mib, 3, kdr, &size, NULL, 0);
}

static int kdebug_trace_setbuf(int nbufs) {
  int mib[4] = {CTL_KERN, KERN_KDEBUG, KERN_KDSETBUF, nbufs};
  return sysctl(mib, 4, NULL, NULL, NULL, 0);
}

static int kdebug_trace_enable(bool enable) {
  int mib[4] = {CTL_KERN, KERN_KDEBUG, KERN_KDENABLE, (int)enable};
  return sysctl(mib, 4, NULL, 0, NULL, 0);
}

static int kdebug_get_bufinfo(kbufinfo_t *info) {
  if (!info) return -1;
  int mib[3] = {CTL_KERN, KERN_KDEBUG, KERN_KDGETBUF};
  size_t needed = sizeof(kbufinfo_t);
  return sysctl(mib, 3, info, &needed, NULL, 0);
}

static int kdebug_trace_read(void *buf, size_t len, size_t *count) {
  if (count) *count = 0;
  if (!buf || !len) return -1;
  int mib[3] = {CTL_KERN, KERN_KDEBUG, KERN_KDREADTR};
  int ret = sysctl(mib, 3, buf, &len, NULL, 0);
  if (ret != 0) return ret;
  *count = len;
  return 0;
}

static int kdebug_wait(size_t timeout_ms, bool *suc) {
  if (timeout_ms == 0) return -1;
  int mib[3] = {CTL_KERN, KERN_KDEBUG, KERN_KDBUFWAIT};
  size_t val = timeout_ms;
  int ret = sysctl(mib, 3, NULL, &val, NULL, 0);
  if (suc) *suc = !!val;
  return ret;
}

// -------------------------------------------------------------------------
// Symbol loading
// -------------------------------------------------------------------------

typedef struct {
  const char *name;
  void **impl;
} kpc__lib_symbol;

#define KPC__NELEMS(x) (sizeof(x) / sizeof((x)[0]))

static const kpc__lib_symbol kpc__symbols_kperf[] = {
    {"kpc_pmu_version",              (void **)&kpc__pmu_version},
    {"kpc_cpu_string",               (void **)&kpc__cpu_string},
    {"kpc_set_counting",             (void **)&kpc__set_counting},
    {"kpc_get_counting",             (void **)&kpc__get_counting},
    {"kpc_set_thread_counting",      (void **)&kpc__set_thread_counting},
    {"kpc_get_thread_counting",      (void **)&kpc__get_thread_counting},
    {"kpc_get_config_count",         (void **)&kpc__get_config_count},
    {"kpc_get_counter_count",        (void **)&kpc__get_counter_count},
    {"kpc_set_config",               (void **)&kpc__set_config},
    {"kpc_get_config",               (void **)&kpc__get_config},
    {"kpc_get_cpu_counters",         (void **)&kpc__get_cpu_counters},
    {"kpc_get_thread_counters",      (void **)&kpc__get_thread_counters},
    {"kpc_force_all_ctrs_set",       (void **)&kpc__force_all_ctrs_set},
    {"kpc_force_all_ctrs_get",       (void **)&kpc__force_all_ctrs_get},
    {"kperf_action_count_set",       (void **)&kperf__action_count_set},
    {"kperf_action_count_get",       (void **)&kperf__action_count_get},
    {"kperf_action_samplers_set",    (void **)&kperf__action_samplers_set},
    {"kperf_action_samplers_get",    (void **)&kperf__action_samplers_get},
    {"kperf_action_filter_set_by_task", (void **)&kperf__action_filter_set_by_task},
    {"kperf_action_filter_set_by_pid",  (void **)&kperf__action_filter_set_by_pid},
    {"kperf_timer_count_set",        (void **)&kperf__timer_count_set},
    {"kperf_timer_count_get",        (void **)&kperf__timer_count_get},
    {"kperf_timer_period_set",       (void **)&kperf__timer_period_set},
    {"kperf_timer_period_get",       (void **)&kperf__timer_period_get},
    {"kperf_timer_action_set",       (void **)&kperf__timer_action_set},
    {"kperf_timer_action_get",       (void **)&kperf__timer_action_get},
    {"kperf_sample_set",             (void **)&kperf__sample_set},
    {"kperf_sample_get",             (void **)&kperf__sample_get},
    {"kperf_reset",                  (void **)&kperf__reset},
    {"kperf_timer_pet_set",          (void **)&kperf__timer_pet_set},
    {"kperf_timer_pet_get",          (void **)&kperf__timer_pet_get},
    {"kperf_ns_to_ticks",            (void **)&kperf__ns_to_ticks},
    {"kperf_ticks_to_ns",            (void **)&kperf__ticks_to_ns},
    {"kperf_tick_frequency",         (void **)&kperf__tick_frequency},
};

static const kpc__lib_symbol kpc__symbols_kperfdata[] = {
    {"kpep_config_create",           (void **)&kpep__config_create},
    {"kpep_config_free",             (void **)&kpep__config_free},
    {"kpep_config_add_event",        (void **)&kpep__config_add_event},
    {"kpep_config_remove_event",     (void **)&kpep__config_remove_event},
    {"kpep_config_force_counters",   (void **)&kpep__config_force_counters},
    {"kpep_config_events_count",     (void **)&kpep__config_events_count},
    {"kpep_config_events",           (void **)&kpep__config_events},
    {"kpep_config_kpc",              (void **)&kpep__config_kpc},
    {"kpep_config_kpc_count",        (void **)&kpep__config_kpc_count},
    {"kpep_config_kpc_classes",      (void **)&kpep__config_kpc_classes},
    {"kpep_config_kpc_map",          (void **)&kpep__config_kpc_map},
    {"kpep_db_create",               (void **)&kpep__db_create},
    {"kpep_db_free",                 (void **)&kpep__db_free},
    {"kpep_db_name",                 (void **)&kpep__db_name},
    {"kpep_db_aliases_count",        (void **)&kpep__db_aliases_count},
    {"kpep_db_aliases",              (void **)&kpep__db_aliases},
    {"kpep_db_counters_count",       (void **)&kpep__db_counters_count},
    {"kpep_db_events_count",         (void **)&kpep__db_events_count},
    {"kpep_db_events",               (void **)&kpep__db_events},
    {"kpep_db_event",                (void **)&kpep__db_event},
    {"kpep_event_name",              (void **)&kpep__event_name},
    {"kpep_event_alias",             (void **)&kpep__event_alias},
    {"kpep_event_description",       (void **)&kpep__event_description},
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
// Public API — Demo 1: profile current thread
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

// -------------------------------------------------------------------------
// Public API — Demo 2: profile an external process by PID
// -------------------------------------------------------------------------

#define KPC__PERF_KPC             6
#define KPC__PERF_KPC_DATA_THREAD 8

static double kpc__get_timestamp(void) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return (double)now.tv_sec + (double)now.tv_usec / 1e6;
}

typedef struct {
  kpc_u32 tid;
  kpc_u64 timestamp_0;
  kpc_u64 timestamp_1;
  kpc_u64 counters_0[KPC_MAX_COUNTERS];
  kpc_u64 counters_1[KPC_MAX_COUNTERS];
} kpc_thread_data;

int kpc_profile_pid(kpc_i32 pid, double profile_time_s, double sample_period_s) {
  if (!kpc__lib_inited || kpc__lib_has_err) {
    kpc__err("kpc: not initialized\n");
    return 1;
  }

  int ret = 0;

  kpep_db *db = NULL;
  if ((ret = kpep__db_create(NULL, &db))) {
    kpc__err("kpc: cannot load pmc database: %d\n", ret);
    return 1;
  }

  kpc__info("kpc: loaded db: %s (%s)\n", db->name, db->marketing_name);
  kpc__info("kpc: tick frequency: %llu\n", (unsigned long long)kperf__tick_frequency());

  kpep_config *cfg = NULL;
  if ((ret = kpep__config_create(db, &cfg))) {
    kpc__err("kpc: failed to create config: %d (%s)\n", ret, kpep_config_error_desc(ret));
    return 1;
  }

  if ((ret = kpep__config_force_counters(cfg))) {
    kpc__err("kpc: failed to force counters: %d (%s)\n", ret, kpep_config_error_desc(ret));
    return 1;
  }

  kpep_event *ev_arr[KPC_EVENT_COUNT] = {0};
  for (int i = 0; i < KPC_EVENT_COUNT; i++) {
    ev_arr[i] = kpc__find_event(db, &kpc_events[i]);
    if (!ev_arr[i]) {
      kpc__err("kpc: event not found: %s\n", kpc_events[i].alias);
      return 1;
    }
    if ((ret = kpep__config_add_event(cfg, &ev_arr[i], 0, NULL))) {
      kpc__err("kpc: failed to add event: %d (%s)\n", ret, kpep_config_error_desc(ret));
      return 1;
    }
  }

  kpc_u32 classes = 0;
  size_t reg_count = 0;
  kpc_config_t regs[KPC_MAX_COUNTERS] = {0};
  size_t counter_map[KPC_MAX_COUNTERS] = {0};

  if (kpep__config_kpc_classes(cfg, &classes) ||
      kpep__config_kpc_count(cfg, &reg_count) ||
      kpep__config_kpc_map(cfg, counter_map, sizeof(counter_map)) ||
      kpep__config_kpc(cfg, regs, sizeof(regs))) {
    kpc__err("kpc: failed to get config\n");
    return 1;
  }

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

  kpc_u32 counter_count = kpc__get_counter_count(classes);
  if (counter_count == 0) {
    kpc__err("kpc: nocounters available\n");
    return 1;
  }

  if (kpc__set_counting(classes) || kpc__set_thread_counting(classes)) {
    kpc__err("kpc: failed to enable counting\n");
    return 1;
  }

  // Set up kperf sampling
  kpc_u32 actionid = 1;
  kpc_u32 timerid = 1;

  kperf__action_count_set(KPERF_ACTION_MAX);
  kperf__timer_count_set(KPERF_TIMER_MAX);
  kperf__action_samplers_set(actionid, KPERF_SAMPLER_PMC_THREAD);
  kperf__action_filter_set_by_pid(actionid, pid);

  kpc_u64 tick = kperf__ns_to_ticks((kpc_u64)(sample_period_s * 1e9));
  kperf__timer_period_set(actionid, tick);
  kperf__timer_action_set(actionid, timerid);
  kperf__timer_pet_set(timerid);
  kperf_lightweight_pet_set(1);
  kperf__sample_set(1);

  // Set up kdebug tracing
  kdebug_reset();
  int nbufs = 1000000;
  kdebug_trace_setbuf(nbufs);
  kdebug_reinit();

  kd_regtype kdr = {0};
  kdr.type = KDBG_VALCHECK;
  kdr.value1 = KDBG_EVENTID(DBG_PERF, KPC__PERF_KPC, KPC__PERF_KPC_DATA_THREAD);
  kdebug_setreg(&kdr);
  kdebug_trace_enable(1);

  // Collect trace data
  size_t buf_capacity = (size_t)nbufs * 2;
  kd_buf *buf_hdr = (kd_buf *)malloc(sizeof(kd_buf) * buf_capacity);
  kd_buf *buf_cur = buf_hdr;
  kd_buf *buf_end = buf_hdr + buf_capacity;

  double begin = kpc__get_timestamp();

  while (buf_hdr) {
    usleep((unsigned int)(2 * sample_period_s * 1e6));

    if ((size_t)(buf_end - buf_cur) < (size_t)nbufs) {
      size_t new_capacity = buf_capacity * 2;
      kd_buf *new_buf = (kd_buf *)realloc(buf_hdr, sizeof(kd_buf) * new_capacity);
      if (!new_buf) { free(buf_hdr); buf_hdr = NULL; break; }
      buf_cur = new_buf + (buf_cur - buf_hdr);
      buf_end = new_buf + new_capacity;
      buf_hdr = new_buf;
      buf_capacity = new_capacity;
    }

    size_t count = 0;
    kdebug_trace_read(buf_cur, sizeof(kd_buf) * (size_t)nbufs, &count);
    for (kd_buf *b = buf_cur, *end = buf_cur + count; b < end; b++) {
      kpc_u32 cls = KDBG_EXTRACT_CLASS(b->debugid);
      kpc_u32 subcls = KDBG_EXTRACT_SUBCLASS(b->debugid);
      kpc_u32 code = KDBG_EXTRACT_CODE(b->debugid);
      if (cls != DBG_PERF || subcls != KPC__PERF_KPC || code != KPC__PERF_KPC_DATA_THREAD)
        continue;
      memmove(buf_cur, b, sizeof(kd_buf));
      buf_cur++;
    }

    if (kpc__get_timestamp() - begin > profile_time_s + sample_period_s)
      break;
  }

  // Cleanup sampling
  kdebug_trace_enable(0);
  kdebug_reset();
  kperf__sample_set(0);
  kperf_lightweight_pet_set(0);
  kpc__set_counting(0);
  kpc__set_thread_counting(0);
  kpc__force_all_ctrs_set(0);

  if (!buf_hdr) {
    kpc__err("kpc: failed to allocate trace buffer\n");
    return 1;
  }

  if (buf_cur - buf_hdr == 0) {
    kpc__err("kpc: nothread PMC data collected\n");
    free(buf_hdr);
    return 1;
  }

  // Aggregate per-thread data
  size_t thread_capacity = 16;
  size_t thread_count = 0;
  kpc_thread_data *thread_data = (kpc_thread_data *)malloc(thread_capacity * sizeof(kpc_thread_data));
  if (!thread_data) {
    kpc__err("kpc: failed to allocate thread data\n");
    free(buf_hdr);
    return 1;
  }

  for (kd_buf *b = buf_hdr; b < buf_cur; b++) {
    kpc_u32 func = b->debugid & KDBG_FUNC_MASK;
    if (func != DBG_FUNC_START) continue;

    kpc_u32 tid = (kpc_u32)b->arg5;
    if (!tid) continue;

    kpc_u32 ci = 0;
    kpc_u64 counters[KPC_MAX_COUNTERS];
    counters[ci++] = b->arg1;
    counters[ci++] = b->arg2;
    counters[ci++] = b->arg3;
    counters[ci++] = b->arg4;

    if (ci < counter_count) {
      for (kd_buf *b2 = b + 1; b2 < buf_cur; b2++) {
        kpc_u32 tid2 = (kpc_u32)b2->arg5;
        if (tid2 != tid) break;
        kpc_u32 func2 = b2->debugid & KDBG_FUNC_MASK;
        if (func2 == DBG_FUNC_START) break;
        if (ci < counter_count) counters[ci++] = b2->arg1;
        if (ci < counter_count) counters[ci++] = b2->arg2;
        if (ci < counter_count) counters[ci++] = b2->arg3;
        if (ci < counter_count) counters[ci++] = b2->arg4;
        if (ci == counter_count) break;
      }
    }

    if (ci != counter_count) continue;

    kpc_thread_data *data = NULL;
    for (size_t i = 0; i < thread_count; i++) {
      if (thread_data[i].tid == tid) { data = &thread_data[i]; break; }
    }

    if (!data) {
      if (thread_count == thread_capacity) {
        thread_capacity *= 2;
        kpc_thread_data *new_data = (kpc_thread_data *)realloc(
            thread_data, thread_capacity * sizeof(kpc_thread_data));
        if (!new_data) {
          kpc__err("kpc: failed to allocate thread data\n");
          free(thread_data);
          free(buf_hdr);
          return 1;
        }
        thread_data = new_data;
      }
      data = &thread_data[thread_count++];
      memset(data, 0, sizeof(kpc_thread_data));
      data->tid = tid;
    }

    if (data->timestamp_0 == 0) {
      data->timestamp_0 = b->timestamp;
      memcpy(data->counters_0, counters, counter_count * sizeof(kpc_u64));
    } else {
      data->timestamp_1 = b->timestamp;
      memcpy(data->counters_1, counters, counter_count * sizeof(kpc_u64));
    }
  }

  // Print results
  kpc_u64 counters_sum[KPC_MAX_COUNTERS] = {0};

  for (size_t i = 0; i < thread_count; i++) {
    kpc_thread_data *d = &thread_data[i];
    if (!d->timestamp_0 || !d->timestamp_1) continue;

    kpc_u64 counters_one[KPC_MAX_COUNTERS] = {0};
    for (size_t c = 0; c < counter_count; c++)
      counters_one[c] = d->counters_1[c] - d->counters_0[c];

    printf("------------------------\n");
    printf("thread: %u, trace time: %f\n", d->tid,
           kperf__ticks_to_ns(d->timestamp_1 - d->timestamp_0) / 1e9);
    for (int e = 0; e < KPC_EVENT_COUNT; e++) {
      kpc_u64 val = counters_one[counter_map[e]];
      printf("%14s: %llu\n", kpc_events[e].alias, (unsigned long long)val);
    }

    for (size_t c = 0; c < counter_count; c++)
      counters_sum[c] += counters_one[c];
  }

  printf("------------------------\n");
  printf("all threads:\n");
  for (int e = 0; e < KPC_EVENT_COUNT; e++) {
    kpc_u64 val = counters_sum[counter_map[e]];
    printf("%14s: %llu\n", kpc_events[e].alias, (unsigned long long)val);
  }

  free(thread_data);
  free(buf_hdr);
  return 0;
}

#endif // KPC_IMPLEMENTATION
