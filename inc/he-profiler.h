/**
 * Heartbeats-EnergyMon profiling interface.
 *
 * @author Connor Imes
 * @date 2015-11-11
 */
#ifndef HE_PROFILER_H
#define HE_PROFILER_H

#include <inttypes.h>

#ifdef HE_PROFILER_DISABLE
  #define HE_PROFILER_EVENT_START() { \
    .time = 0, \
    .energy = 0, \
  }
  // dummy operations to suppress unused variable warnings
  #define HE_PROFILER_EVENT_END(start, id, tag, work) \
    do { \
      (void) start; \
      (void) id; \
      (void) tag; \
      (void) work; \
    } while (0)
#else
  #define HE_PROFILER_EVENT_START() { \
    .time = he_profiler_get_time(), \
    .energy = he_profiler_get_energy(), \
  }
  #define HE_PROFILER_EVENT_END(start, id, tag, work) \
    he_profiler_event(id, tag, work, \
                      start.time, he_profiler_get_time(), \
                      start.energy, he_profiler_get_energy());
#endif

typedef struct he_time_energy {
  uint64_t time;
  uint64_t energy;
} he_time_energy;

/**
 * Get the current time in nanoseconds.
 */
uint64_t he_profiler_get_time(void);

/**
 * Get the current energy reading in microjoules.
 */
uint64_t he_profiler_get_energy(void);

/**
 * Initialize the profiler.
 *
 * @param num_profilers
 * @param application_profiler
 * @param profiler_names
 * @param default_window_size
 * @param env_var_prefix
 * @param log_path
 *
 * @return 0 on success, something else otherwise
 */
int he_profiler_init(unsigned int num_profilers,
                     int application_profiler,
                     const char** profiler_names,
                     uint64_t default_window_size,
                     const char* env_var_prefix,
                     const char* log_path);

/**
 * Profile an event.
 *
 * @param profiler
 * @param id
 * @param work
 * @param time_start
 * @param time_end
 * @param energy_start
 * @param energy_end
 *
 * @return 0 on success, something else otherwise
 */
int he_profiler_event(unsigned int profiler,
                      uint64_t id,
                      uint64_t work,
                      uint64_t time_start,
                      uint64_t time_end,
                      uint64_t energy_start,
                      uint64_t energy_end);

/**
 * Cleanup the profiler.
 *
 * @return 0 on success, something else otherwise
 */
int he_profiler_finish(void);

#endif
