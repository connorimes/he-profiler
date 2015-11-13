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
  int init = he_profiler_init(NUM_PROFILERS,
                              APPLICATION,
                              profiler_names,
                              default_window_size,
                              env_var_prefix,
                              log_path);
  assert(init == 0);
  he_time_energy event_start = HE_PROFILER_EVENT_START();
  HE_PROFILER_EVENT_END(event_start, TEST, TEST, 1);
  assert(he_profiler_finish() == 0);
  return 0;
}
