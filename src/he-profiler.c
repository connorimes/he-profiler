/**
 * Heartbeats-EnergyMon profiling implementation.
 *
 * @author Connor Imes
 * @date 2015-11-11
 */
#include <energymon-default.h>
#include <errno.h>
#include <fcntl.h>
#include <heartbeat-pow-container.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "he-profiler.h"

#ifndef HE_PROFILER_POLLER_MIN_SLEEP_US
  // set a min polling interval for fast monitors - 10 ms = 100 reads/sec
  #define HE_PROFILER_POLLER_MIN_SLEEP_US 10000
#endif

typedef struct he_profiler_poller {
  volatile int run;
  unsigned int idx;
  uint64_t min_sleep_us;
  pthread_t thread;
} he_profiler_poller;

typedef struct he_profiler_container {
  unsigned int num_hbs;
  heartbeat_pow_container* heartbeats;
  energymon* em;
} he_profiler_container;

// global container
static he_profiler_container hepc = {
  .num_hbs = 0,
  .heartbeats = NULL,
  .em = NULL,
};

// a single application-level profiler that runs at fixed intervals
static he_profiler_poller app_profiler = {
  .run = 0,
  .idx = 0,
  .min_sleep_us = 0,
};

static int he_profiler_container_finish(he_profiler_container* hpc);

static inline uint64_t he_profiler_get_time(void) {
  struct timespec ts;
#ifdef __MACH__
  // OS X does not have clock_gettime, use clock_get_time
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts.tv_sec = mts.tv_sec;
  ts.tv_nsec = mts.tv_nsec;
#else
  // CLOCK_REALTIME is always supported, this should never fail
  clock_gettime(CLOCK_REALTIME, &ts);
#endif
  // must use a const or cast a literal - using a simple literal can overflow!
  const uint64_t ONE_BILLION = 1000000000;
  return ts.tv_sec * ONE_BILLION + ts.tv_nsec;
}

static inline uint64_t he_profiler_get_energy(void) {
  if (hepc.em == NULL) {
    errno = EINVAL;
    return 0;
  }
  errno = 0;
  uint64_t energy = hepc.em->fread(hepc.em);
  if (energy == 0 && errno) {
    perror("Error reading from energy monitor");
  }
  return energy;
}

static void* application_profiler(void* args) {
  (void) args; // silence the compiler
  // energymon refresh interval can limit the profiling rate
  uint64_t em_interval_us = hepc.em->finterval(hepc.em);
  // TODO: Use clock_nanosleep
  useconds_t sleep_us = em_interval_us < app_profiler.min_sleep_us ?
    app_profiler.min_sleep_us : em_interval_us;

  // profile at intervals until we're told to stop
  he_profiler_event event;
  uint64_t i;
  he_profiler_event_begin(&event);
  for (i = 0; app_profiler.run; i++) {
    usleep(sleep_us);
    he_profiler_event_end_begin(&event, app_profiler.idx, i, 1);
  }

  return (void*) NULL;
}

static inline int init_heartbeat(heartbeat_pow_container* hc,
                                 uint64_t window_size,
                                 const char* name,
                                 const char* log_path) {
  char log[1024];
  int fd = 0;
  int err_save;
  if (name != NULL) {
    // open the log file
    log_path = log_path == NULL ? "." : log_path;
    snprintf(log, sizeof(log), "%s/heartbeat-%s.log", log_path, name);
    fd = open(log, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd <= 0) {
      perror(log);
      return -1;
    }
  }
  if (heartbeat_pow_container_init_context(hc, window_size, fd, NULL)) {
    perror("Failed to initialize heartbeat");
    if (fd > 0) {
      err_save = errno;
      if (close(fd)) {
        perror(log);
      }
      errno = err_save;
    }
    return -1;
  }
  // write header to log file if we have one
  if (fd > 0 && hb_pow_log_header(fd)) {
    perror(log);
    err_save = errno;
    heartbeat_pow_container_finish(hc);
    if (close(fd)) {
      perror(log);
    }
    errno = err_save;
    return -1;
  }
  return 0;
}

static int he_profiler_container_init(he_profiler_container* hpc,
                                      unsigned int num_profilers,
                                      const char* const* profiler_names,
                                      const uint64_t* window_sizes,
                                      uint64_t default_window_size,
                                      const char* log_path) {
  unsigned int i;
  uint64_t window_size;
  const char* pname;
  energymon* em;
  int err_save;

  // zero-out for safety during failure cleanup
  memset(hpc, 0, sizeof(he_profiler_container));

  // initialize heartbeats
  hpc->heartbeats = calloc(num_profilers, sizeof(heartbeat_pow_container));
  if (hpc->heartbeats == NULL) {
    return -1;
  }
  hpc->num_hbs = num_profilers;
  for (i = 0; i < hpc->num_hbs; i++) {
    window_size = (window_sizes == NULL || window_sizes[i] == 0) ?
      default_window_size : window_sizes[i];
    pname = profiler_names == NULL ? NULL : profiler_names[i];
    if (init_heartbeat(&hpc->heartbeats[i], window_size, pname, log_path)) {
      err_save = errno;
      he_profiler_container_finish(hpc);
      errno = err_save;
      return -1;
    }
  }

  // start energy monitoring tool
  em = malloc(sizeof(energymon));
  if (em == NULL) {
    err_save = errno;
    he_profiler_container_finish(hpc);
    errno = err_save;
    return -1;
  }
  if (energymon_get_default(em) || em->finit(em)) {
    perror("Failed to get/initialize energymon");
    err_save = errno;
    free(em);
    he_profiler_container_finish(hpc);
    errno = err_save;
    return -1;
  }
  hpc->em = em;

  return 0;
}

int he_profiler_init(unsigned int num_profilers,
                     const char* const* profiler_names,
                     const uint64_t* window_sizes,
                     uint64_t default_window_size,
                     unsigned int app_profiler_id,
                     uint64_t app_profiler_min_sleep_us,
                     const char* log_path) {
  int err_save;

  if (hepc.heartbeats != NULL || hepc.num_hbs != 0 || hepc.em != NULL) {
    errno = EINVAL;
    fprintf(stderr, "Profiler already initialized\n");
    return -1;
  }

  if (num_profilers == 0 || default_window_size == 0) {
    errno = EINVAL;
    return -1;
  }

  if (he_profiler_container_init(&hepc, num_profilers, profiler_names,
                                 window_sizes, default_window_size, log_path)) {
    return -1;
  }

  // start thread that profiles entire application execution
  if (app_profiler_id < num_profilers) {
      app_profiler.run = 1;
      app_profiler.idx = app_profiler_id;
      app_profiler.min_sleep_us = app_profiler_min_sleep_us == 0 ?
        HE_PROFILER_POLLER_MIN_SLEEP_US : app_profiler_min_sleep_us;
      errno = pthread_create(&app_profiler.thread, NULL, &application_profiler,
                             NULL);
      if (errno) {
        perror("Failed to create application profiler thread");
        err_save = errno;
        app_profiler.run = 0;
        he_profiler_finish();
        errno = err_save;
        return -1;
      }
    }

  return 0;
}

int he_profiler_event_begin(he_profiler_event* event) {
  if (hepc.heartbeats == NULL) {
    fprintf(stderr, "Profiler not initialized\n");
    errno = EINVAL;
    return -1;
  }
  if (event == NULL) {
    errno = EINVAL;
    return -1;
  }
  event->start_time = he_profiler_get_time();
  errno = 0;
  event->start_energy = he_profiler_get_energy();
  return errno;
}

static inline int he_profiler_event_issue_local(he_profiler_event* event,
                                                unsigned int profiler,
                                                uint64_t id,
                                                uint64_t work,
                                                int update) {
  if (hepc.heartbeats == NULL) {
    fprintf(stderr, "Profiler not initialized\n");
    errno = EINVAL;
    return -1;
  }
  if (profiler >= hepc.num_hbs) {
    // TODO: We could make this a public assertion instead
    fprintf(stderr, "Profiler out of range: %d\n", profiler);
    errno = EINVAL;
    return -1;
  }
  if (event == NULL) {
    errno = EINVAL;
    return -1;
  }
  if (update) {
    event->end_time = he_profiler_get_time();
    event->end_energy = he_profiler_get_energy();
  }
  heartbeat_pow(&hepc.heartbeats[profiler].hb, id, work,
                event->start_time, event->end_time,
                event->start_energy, event->end_energy);
  return 0;
}

int he_profiler_event_end(he_profiler_event* event,
                          unsigned int profiler,
                          uint64_t id,
                          uint64_t work) {
  return he_profiler_event_issue_local(event, profiler, id, work, 1);
}

int he_profiler_event_end_begin(he_profiler_event* event,
                                unsigned int profiler,
                                uint64_t id,
                                uint64_t work) {
  int ret = he_profiler_event_end(event, profiler, id, work);
  if (!ret && event != NULL) {
    event->start_time = event->end_time;
    event->start_energy = event->end_energy;
  }
  return ret;
}

int he_profiler_event_issue(he_profiler_event* event,
                            unsigned int profiler,
                            uint64_t id,
                            uint64_t work) {
  return he_profiler_event_issue_local(event, profiler, id, work, 0);
}

static inline int finish_heartbeat(heartbeat_pow_container* hc) {
  int err_save = 0;
  int fd = hb_pow_get_log_fd(&hc->hb);
  if (fd > 0) {
    // write remaining log data and close log file
    if (hb_pow_log_window_buffer(&hc->hb, fd)) {
      err_save = errno;
    }
    if (close(fd)) {
      err_save = errno;
    }
  }
  heartbeat_pow_container_finish(hc);
  errno = err_save;
  return err_save;
}

static int he_profiler_container_finish(he_profiler_container* hpc) {
  int err_save = 0;
  unsigned int i;
  unsigned int nhbs;
  heartbeat_pow_container* hcs;
  energymon* em;

  // finish heartbeats
  nhbs = __sync_lock_test_and_set(&hpc->num_hbs, 0);
  hcs = __sync_lock_test_and_set(&hpc->heartbeats, NULL);
  if (hcs != NULL) {
    for (i = 0; i < nhbs; i++) {
      if (finish_heartbeat(&hcs[i])) {
        perror("Error finishing heartbeat");
        err_save = errno;
      }
    }
    free(hcs);
  }

  // stop/cleanup energymon
  em = __sync_lock_test_and_set(&hpc->em, NULL);
  if (em != NULL && em->ffinish(em)) {
    perror("Failed to finish energymon");
    err_save = errno;
  }
  free(em);

  errno = err_save ? err_save : errno;
  return err_save;
}

int he_profiler_finish(void) {
  int err_save = 0;
  // stop application profiler thread
  if (__sync_lock_test_and_set(&app_profiler.run, 0)) {
    errno = pthread_join(app_profiler.thread, NULL);
    if (errno) {
      perror("Failed to join application profiler thread");
      err_save = errno;
    }
  }
  // finish containers
  if (he_profiler_container_finish(&hepc)) {
    err_save = errno;
  }
  return err_save;
}
