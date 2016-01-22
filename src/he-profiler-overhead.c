/**
 * This utility tests the overhead of a the background application profiler.
 *
 * @author Connor Imes
 * @date 2015-12-08
 */
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "he-profiler.h"

enum OVERHEAD_PROFILER {
  APPLICATION = 0,
  EVENT,
  PROFILER_COUNT
};

static inline double get_event_watts(he_profiler_event* event) {
  // Watts = microjoules * 1000 / nanoseconds
  return (event->end_energy - event->start_energy) * 1000.0 /
         (event->end_time - event->start_time);
}

static inline int event_nanosleep(he_profiler_event* event,
                                  const struct timespec* ts) {
  // begin event
  he_profiler_event_begin(event);
  // sleep
  if (nanosleep(ts, NULL)) {
    perror("Sleep failed");
    return -1;
  }
  // end event
  return he_profiler_event_end(event, EVENT, 1, 0);
}

int he_profiler_overhead_exec(int use_app_profiler,
                              const char* app_profiler_name,
                              uint64_t window_size,
                              const char* log_path,
                              uint64_t sleep_us,
                              double* watts) {
  int ret;
  struct timespec ts;
  int app_profiler = use_app_profiler ? APPLICATION : -1;
  const char* profiler_names[] = { NULL, app_profiler_name };
  he_profiler_event event;

  ts.tv_sec = sleep_us / (1000 * 1000);
  ts.tv_nsec = (sleep_us % (1000 * 1000) * 1000);

  // initialize profilers
  if (he_profiler_init(PROFILER_COUNT, profiler_names, NULL, window_size,
                       app_profiler, 0, log_path)) {
    return -1;
  }
  // sleep
  ret = event_nanosleep(&event, &ts);
  *watts = get_event_watts(&event);

  // always cleanup profilers
  if (he_profiler_finish()) {
    ret = -1;
  }

  return ret;
}

int he_profiler_overhead_power_diff(uint64_t window_size,
                                    const char* log_path,
                                    const char* app_profiler_name,
                                    uint64_t sleep_us,
                                    double* power_app,
                                    double* power_noapp) {
  // first run with the application profiler
  if (he_profiler_overhead_exec(1, app_profiler_name, window_size, log_path,
                                sleep_us, power_app)) {
    return -1;
  }
  // now run without the application profiler
  if (he_profiler_overhead_exec(0, NULL, window_size, NULL, sleep_us,
                                power_noapp)) {
    return -1;
  }
  return 0;
}

int main(void) {
  double pwr_diff = 0.0;
  double power_app;
  double power_noapp;
  double pwr_diff_total = 0.0;
  uint64_t window_size = 20;
  uint64_t sleep_us = 1000000;
  unsigned int iterations = 20;
  unsigned int i;

  // a warmup call
  he_profiler_overhead_power_diff(window_size, NULL, "APPLICATION", sleep_us,
                                  &power_app, &power_noapp);
  for (i = 0; i < iterations; i++) {
    if (he_profiler_overhead_power_diff(window_size, NULL, "APPLICATION",
                                        sleep_us, &power_app, &power_noapp)) {
      return -1;
    }
    pwr_diff = power_app - power_noapp;
    pwr_diff_total += pwr_diff;
    printf("%f\n", pwr_diff);
  }
  printf("AVERAGE: %f\n", pwr_diff_total / iterations);

  return 0;
}
