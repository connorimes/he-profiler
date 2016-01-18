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
  const uint64_t default_window_size = 20;
  const char* env_var_prefix = NULL;
  const char* log_path = NULL;
#endif
  int init = HE_PROFILER_INIT(NUM_PROFILERS,
                              APPLICATION,
                              profiler_names,
                              default_window_size,
                              env_var_prefix,
                              log_path);
  assert(init == 0);
  errno = 0;
  HE_PROFILER_EVENT_BEGIN(event1);
  assert(errno = 0);
  HE_PROFILER_EVENT_BEGIN(event2);
  assert(errno = 0);
  assert(HE_PROFILER_EVENT_END_BEGIN(TEST, TEST, 1, event1) == 0);
  assert(HE_PROFILER_EVENT_END(TEST, TEST, 2, event1) == 0);
  assert(HE_PROFILER_EVENT_END(TEST, TEST, 3, event2) == 0);
  assert(HE_PROFILER_FINISH() == 0);
  return 0;
}
