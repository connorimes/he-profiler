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
                     int application_profiler,
                     const char** profiler_names,
                     uint64_t default_window_size,
                     const char* env_var_prefix,
                     const char* log_path) {
  UNUSED(num_profilers);
  UNUSED(application_profiler);
  UNUSED(profiler_names);
  UNUSED(default_window_size);
  UNUSED(env_var_prefix);
  UNUSED(log_path);
  return 0;
}

int he_profiler_event_begin(he_profiler_event* event) {
  UNUSED(event);
  return 0;
}

int he_profiler_event_end(unsigned int profiler,
                          uint64_t id,
                          uint64_t work,
                          he_profiler_event* event) {
  UNUSED(profiler);
  UNUSED(id);
  UNUSED(work);
  UNUSED(event);
  return 0;
}

int he_profiler_event_end_begin(unsigned int profiler,
                                uint64_t id,
                                uint64_t work,
                                he_profiler_event* event) {
  UNUSED(profiler);
  UNUSED(id);
  UNUSED(work);
  UNUSED(event);
  return 0;
}

int he_profiler_finish(void) {
  return 0;
}
