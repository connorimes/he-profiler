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

#include <errno.h>
#include <inttypes.h>

#ifdef HE_PROFILER_ENABLE

#define HE_PROFILER_INIT(num_profilers, \
                         profiler_names, \
                         window_sizes, \
                         default_window_size, \
                         app_profiler_id, \
                         app_profiler_min_sleep_us, \
                         log_path) \
  he_profiler_init(num_profilers, \
                   profiler_names, \
                   window_sizes, \
                   default_window_size, \
                   app_profiler_id, \
                   app_profiler_min_sleep_us, \
                   log_path)

#define HE_PROFILER_EVENT_BEGIN_R(event) \
  he_profiler_event_begin(&event)

#define HE_PROFILER_EVENT_BEGIN(event) \
  he_profiler_event event; \
  errno = he_profiler_event_begin(&event)

#define HE_PROFILER_EVENT_END(event, profiler, id, work) \
  he_profiler_event_end(&event, profiler, id, work)

#define HE_PROFILER_EVENT_END_BEGIN(event, profiler, id, work) \
  he_profiler_event_end_begin(&event, profiler, id, work)

#define HE_PROFILER_FINISH() he_profiler_finish()

#else

static inline int __he_profiler_dummy(void) { return 0; }

#define HE_PROFILER_INIT(num_profilers, \
                         profiler_names, \
                         window_sizes, \
                         default_window_size, \
                         app_profiler_id, \
                         app_profiler_min_sleep_us, \
                         log_path) (0)

#define HE_PROFILER_EVENT_BEGIN_R(event) __he_profiler_dummy()

#define HE_PROFILER_EVENT_BEGIN(event) __he_profiler_dummy()

#define HE_PROFILER_EVENT_END(event, profiler, id, work) __he_profiler_dummy()

#define HE_PROFILER_EVENT_END_BEGIN(event, profiler, id, work) __he_profiler_dummy()

#define HE_PROFILER_FINISH() __he_profiler_dummy()

#endif

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
 * @param profiler_names
 * @param window_sizes
 * @param default_window_size
 * @param app_profiler_id
 * @param app_profiler_min_sleep_us
 * @param log_path
 *
 * @return 0 on success, something else otherwise
 */
int he_profiler_init(unsigned int num_profilers,
                     const char* const* profiler_names,
                     const uint64_t* window_sizes,
                     uint64_t default_window_size,
                     unsigned int app_profiler_id,
                     uint64_t app_profiler_min_sleep_us,
                     const char* log_path);

/**
 * Begin an event by fetching the start time and energy values.
 *
 * @param event
 *
 * @return 0 on success, something else otherwise
 */
int he_profiler_event_begin(he_profiler_event* event);

/**
 * End an event by fetching the end time and energy values.
 * Issues a heartbeat.
 *
 * @param event
 * @param profiler
 * @param id
 * @param work
 *
 * @return 0 on success, something else otherwise
 */
int he_profiler_event_end(he_profiler_event* event,
                          unsigned int profiler,
                          uint64_t id,
                          uint64_t work);

/**
 * End an event and begin the next (the end values are the start of the next).
 * Issues a heartbeat.
 *
 * @param event
 * @param profiler
 * @param id
 * @param work
 *
 * @return 0 on success, something else otherwise
 */
int he_profiler_event_end_begin(he_profiler_event* event,
                                unsigned int profiler,
                                uint64_t id,
                                uint64_t work);

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
