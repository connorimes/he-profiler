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
const uint64_t default_window_size = 20;
const char* env_var_prefix = NULL;
const char* log_path = NULL;

int main(int argc, char** argv) {
  he_profiler_event event;
  int init = he_profiler_init(NUM_PROFILERS,
                              APPLICATION,
                              profiler_names,
                              default_window_size,
                              env_var_prefix,
                              log_path);
  assert(init == 0);
  he_profiler_event_begin(&event);
  he_profiler_event_end(TEST, TEST, 1, &event);
  he_profiler_event_end_begin(TEST, TEST, 1, &event);
  he_profiler_event_end(TEST, TEST, 2, &event);
  assert(he_profiler_finish() == 0);
  return 0;
}
