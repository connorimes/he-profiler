#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include "he-profiler.h"

typedef enum PROFILERS {
  APPLICATION,
  TEST,
  NUM_PROFILERS
} PROFILERS;

int main(void) {
#ifdef HE_PROFILER_ENABLE // disables unused variable warnings
  const char* profiler_names[] = {"application", "test"};
  const uint64_t* window_sizes = NULL;
  const uint64_t default_window_size = 20;
  const uint64_t min_app_profiler_sleep_us = 0;
  const char* log_path = NULL;
  he_profiler_event event2;
#endif
  int init = HE_PROFILER_INIT(NUM_PROFILERS,
                              profiler_names,
                              window_sizes,
                              default_window_size,
                              APPLICATION,
                              min_app_profiler_sleep_us,
                              log_path);
  assert(init == 0);
  errno = 0;
  HE_PROFILER_EVENT_BEGIN(event1);
  assert(errno == 0);
  HE_PROFILER_EVENT_BEGIN_R(event2);
  assert(errno == 0);
  assert(HE_PROFILER_EVENT_END_BEGIN(event1, TEST, TEST, 1) == 0);
  assert(HE_PROFILER_EVENT_END(event1, TEST, TEST, 2) == 0);
  assert(HE_PROFILER_EVENT_ISSUE(event2, TEST, TEST, 3) == 0);
  assert(HE_PROFILER_FINISH() == 0);
  return 0;
}
