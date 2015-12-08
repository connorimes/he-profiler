/**
 * Heartbeats-EnergyMon profiling implementation.
 *
 * @author Connor Imes
 * @date 2015-11-11
 */
#include <energymon/energymon-default.h>
#include <fcntl.h>
#include <heartbeats-simple/heartbeat-pow-container.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "he-profiler.h"

#ifndef HE_PROFILER_POLLER_MIN_SLEEP_US
  // set a min polling interval for fast monitors - 50 ms = 20 reads/sec
  #define HE_PROFILER_POLLER_MIN_SLEEP_US 50000
#endif

typedef struct he_profiler_poller {
  int run;
  unsigned int idx;
  pthread_t thread;
} he_profiler_poller;

// global heartbeat data
static heartbeat_pow_container* heartbeats = NULL;
static unsigned int num_hbs = 0;
// global energymon instance
static energymon* em = NULL;
// a single application-level profiler that runs at fixed intervals
static he_profiler_poller app_profiler = {
  .run = 0,
  .idx = 0,
};

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
  clock_gettime(CLOCK_REALTIME, &ts);
#endif
  return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

static inline uint64_t he_profiler_get_energy(void) {
  return em == NULL ? 0 : em->fread(em);
}

static void* application_profiler(void* args) {
  uint64_t min_sleep_us = (uint64_t) args;
  // energymon refresh interval can limit the profiling rate
  uint64_t em_interval_us = em->finterval(em);
  useconds_t sleep_us = em_interval_us < min_sleep_us ?
    min_sleep_us : em_interval_us;

  // profile at intervals until we're told to stop
  he_profiler_event event;
  uint64_t i;
  he_profiler_event_begin(&event);
  for (i = 0; app_profiler.run; i++) {
    usleep(sleep_us);
    he_profiler_event_end_begin(app_profiler.idx, i, 1, &event);
  }

  return (void*) NULL;
}

static inline int init_heartbeat(heartbeat_pow_container* hc,
                                 uint64_t window_size,
                                 const char* name,
                                 const char* log_path) {
  char heartbeat_log[1024];
  int fd = 0;
  if (name != NULL) {
    // open the log file
    log_path = log_path == NULL ? "." : log_path;
    snprintf(heartbeat_log, sizeof(heartbeat_log), "%s/heartbeat-%s.log",
             log_path, name);
    fd = open(heartbeat_log,
              O_CREAT|O_WRONLY|O_TRUNC,
              S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd <= 0) {
      fprintf(stderr, "Failed to open heartbeat log\n");
      return -1;
    }
  }
  if (heartbeat_pow_container_init_context(hc, window_size, fd, NULL)) {
    fprintf(stderr, "Failed to initialize heartbeat\n");
    if (fd > 0) {
      close(fd);
    }
    return -1;
  }
  // write header to log file if we have one
  if (fd > 0 && hb_pow_log_header(fd)) {
    fprintf(stderr, "Failed to write heartbeat log header\n");
    heartbeat_pow_container_finish(hc);
    close(fd);
    return -1;
  }
  return 0;
}

int he_profiler_init(unsigned int num_profilers,
                     int app_profiler_id,
                     const char** profiler_names,
                     uint64_t default_window_size,
                     const char* env_var_prefix,
                     const char* log_path) {
  unsigned int i;
  char env_var[1024];
  const char* pname = NULL;
  uint64_t window_size = default_window_size;
  uint64_t min_sleep_us = HE_PROFILER_POLLER_MIN_SLEEP_US;
  char ev_prefix[1024] = { '\0' };
  if (env_var_prefix != NULL) {
    // append a '_' if one isn't there
    snprintf(ev_prefix, sizeof(ev_prefix), "%s%c", env_var_prefix,
             env_var_prefix[strlen(env_var_prefix) - 1] == '_' ? '\0' : '_');
  }

  if (heartbeats != NULL || num_hbs != 0 || em != NULL) {
    fprintf(stderr, "Profiler already initialized\n");
    return -1;
  }

  if (num_profilers == 0) {
    fprintf(stderr, "num_profilers must be > 0\n");
    return -1;
  }

  // read configurations from environment
  snprintf(env_var, sizeof(env_var), "%sWINDOW_SIZE", ev_prefix);
  if (getenv(env_var) != NULL) {
    window_size = strtoull(getenv(env_var), NULL, 0);
  }
  snprintf(env_var, sizeof(env_var), "%sMIN_SLEEP_US", ev_prefix);
  if (getenv(env_var) != NULL) {
    min_sleep_us = strtoull(getenv(env_var), NULL, 0);
  }

  // create heartbeat containers
  heartbeats = calloc(num_profilers, sizeof(heartbeat_pow_container));
  if (heartbeats == NULL) {
    fprintf(stderr, "Failed to calloc profilers\n");
    return -1;
  }
  num_hbs = num_profilers;

  // start energy monitoring tool
  em = malloc(sizeof(energymon));
  if (em == NULL) {
    fprintf(stderr, "Failed to malloc energymon\n");
    free(heartbeats);
    return -1;
  }
  if (energymon_get_default(em) || em->finit(em)) {
    fprintf(stderr, "Failed to initialize energymon\n");
    he_profiler_finish();
    return -1;
  }

  // initialize heartbeats
  for (i = 0; i < num_hbs; i++) {
    pname = profiler_names == NULL ? NULL : profiler_names[i];
    if (init_heartbeat(&heartbeats[i], window_size, pname, log_path)) {
      he_profiler_finish();
      return -1;
    }
  }

  // start thread that profiles entire application execution
  app_profiler.idx = app_profiler_id;
  if (app_profiler_id >= 0) {
      app_profiler.run = 1;
      if (pthread_create(&app_profiler.thread, NULL, &application_profiler,
                         (void*) min_sleep_us)) {
        fprintf(stderr, "Failed to create application profiler thread\n");
        app_profiler.run = 0;
        he_profiler_finish();
        return -1;
      }
    }

  return 0;
}

void he_profiler_event_begin(he_profiler_event* event) {
  if (event != NULL) {
    event->start_time = he_profiler_get_time();
    event->start_energy = he_profiler_get_energy();
  }
}

int he_profiler_event_end(unsigned int profiler,
                          uint64_t id,
                          uint64_t work,
                          he_profiler_event* event) {
  if (heartbeats == NULL) {
    fprintf(stderr, "Profiler not initialized\n");
    return -1;
  }
  if (profiler >= num_hbs) {
    fprintf(stderr, "Profiler out of range: %d\n", profiler);
    return -1;
  }
  if (event == NULL) {
    return -1;
  }
  event->end_time = he_profiler_get_time();
  event->end_energy = he_profiler_get_energy();
  heartbeat_pow(&heartbeats[profiler].hb, id, work,
                event->start_time, event->end_time,
                event->start_energy, event->end_energy);
  return 0;
}

int he_profiler_event_end_begin(unsigned int profiler,
                                uint64_t id,
                                uint64_t work,
                                he_profiler_event* event) {
  int ret = he_profiler_event_end(profiler, id, work, event);
  if (!ret && event != NULL) {
    event->start_time = event->end_time;
    event->start_energy = event->end_energy;
  }
  return ret;
}



static inline int finish_heartbeat(heartbeat_pow_container* hc) {
  int ret = 0;
  int fd = hb_pow_get_log_fd(&hc->hb);
  if (fd > 0) {
    // write remaining log data and close log file
    if (hb_pow_log_window_buffer(&hc->hb, fd)) {
      fprintf(stderr, "Failed to write remaining heartbeat data to log\n");
      ret += -1;
    }
    if (close(fd)) {
      fprintf(stderr, "Failed to close heartbeat log\n");
      ret += -1;
    }
  }
  heartbeat_pow_container_finish(hc);
  return ret;
}

int he_profiler_finish(void) {
  unsigned int i;
  int ret = 0;

  // stop application profiler thread
  if (__sync_lock_test_and_set(&app_profiler.run, 0)) {
    if (pthread_join(app_profiler.thread, NULL)) {
      fprintf(stderr, "Failed to join application profiler thread\n");
      ret += -1;
    }
  }

  // finish heartbeats
  unsigned int nhbs_tmp = __sync_lock_test_and_set(&num_hbs, 0);
  heartbeat_pow_container* hb_tmp = __sync_lock_test_and_set(&heartbeats, NULL);
  if (hb_tmp != NULL) {
    for (i = 0; i < nhbs_tmp; i++) {
      ret += finish_heartbeat(&hb_tmp[i]);
    }
    free(hb_tmp);
  }

  // stop/cleanup energymon
  energymon* em_tmp = __sync_lock_test_and_set(&em, NULL);
  if (em_tmp != NULL && em_tmp->ffinish(em_tmp)) {
    fprintf(stderr, "Failed to finish energymon\n");
    ret += -1;
  }

  return ret;
}
