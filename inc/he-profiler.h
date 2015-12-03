/**
 * Heartbeats-EnergyMon profiling interface.
 *
 * @author Connor Imes
 * @date 2015-11-11
 */
#ifndef HE_PROFILER_H
#define HE_PROFILER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

typedef struct he_profiler_event {
  uint64_t start_time;
  uint64_t start_energy;
  uint64_t end_time;
  uint64_t end_energy;
} he_profiler_event;

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
 * Begin an event by fetching the start time and energy values.
 *
 * @param event
 */
void he_profiler_event_begin(he_profiler_event* event);

/**
 * End an event by fetching the end time and energy values.
 * Issues a heartbeat.
 *
 * @param profiler
 * @param id
 * @param work
 * @param event
 *
 * @return 0 on success, something else otherwise
 */
int he_profiler_event_end(unsigned int profiler,
                          uint64_t id,
                          uint64_t work,
                          he_profiler_event* event);

/**
 * End an event and begin the next (the end values are the start of the next).
 * Issues a heartbeat.
 *
 * @param profiler
 * @param id
 * @param work
 * @param event
 *
 * @return 0 on success, something else otherwise
 */
int he_profiler_event_end_begin(unsigned int profiler,
                                uint64_t id,
                                uint64_t work,
                                he_profiler_event* event);

/**
 * Cleanup the profiler.
 *
 * @return 0 on success, something else otherwise
 */
int he_profiler_finish(void);

#ifdef __cplusplus
}
#endif

#endif
