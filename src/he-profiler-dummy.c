/**
 * This dummy implementation can be substituted for the real one when profiling
 * needs to be disabled.
 *
 * @author Connor Imes
 * @date 2016-01-14
 */
#include <inttypes.h>
#include "he-profiler.h"

#define UNUSED(x) (void)(x)

int he_profiler_init(unsigned int num_profilers,
                     const char* const* profiler_names,
                     const uint64_t* window_sizes,
                     uint64_t default_window_size,
                     unsigned int app_profiler_id,
                     uint64_t app_profiler_min_sleep_us,
                     const char* log_path) {
  UNUSED(num_profilers);
  UNUSED(profiler_names);
  UNUSED(window_sizes);
  UNUSED(default_window_size);
  UNUSED(app_profiler_id);
  UNUSED(app_profiler_min_sleep_us);
  UNUSED(log_path);
  return 0;
}

int he_profiler_event_begin(he_profiler_event* event) {
  UNUSED(event);
  return 0;
}

int he_profiler_event_end(he_profiler_event* event,
                          unsigned int profiler,
                          uint64_t id,
                          uint64_t work) {
  UNUSED(event);
  UNUSED(profiler);
  UNUSED(id);
  UNUSED(work);
  return 0;
}

int he_profiler_event_end_begin(he_profiler_event* event,
                                unsigned int profiler,
                                uint64_t id,
                                uint64_t work) {
  UNUSED(event);
  UNUSED(profiler);
  UNUSED(id);
  UNUSED(work);
  return 0;
}

int he_profiler_event_issue(he_profiler_event* event,
                            unsigned int profiler,
                            uint64_t id,
                            uint64_t work) {
  UNUSED(event);
  UNUSED(profiler);
  UNUSED(id);
  UNUSED(work);
  return 0;
}

int he_profiler_finish(void) {
  return 0;
}
