#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include "he-profiler.h"

typedef enum PROFILERS {
  APPLICATION,
  TEST,
  NUM_PROFILERS
} PROFILERS;

const char* profiler_names[] = {"application", "test"};
const uint64_t* window_sizes = NULL;
const uint64_t default_window_size = 20;
const uint64_t min_app_profiler_sleep_us = 0;
const char* log_path = NULL;

int main(void) {
  he_profiler_event event;
  int init = he_profiler_init(NUM_PROFILERS,
                              profiler_names,
                              window_sizes,
                              default_window_size,
                              APPLICATION,
                              min_app_profiler_sleep_us,
                              log_path);
  assert(init == 0);
  assert(he_profiler_event_begin(&event) == 0);
  assert(he_profiler_event_end(TEST, TEST, 1, &event) == 0);
  assert(he_profiler_event_end_begin(TEST, TEST, 1, &event) == 0);
  assert(he_profiler_event_end(TEST, TEST, 2, &event) == 0);
  assert(he_profiler_finish() == 0);
  return 0;
}
