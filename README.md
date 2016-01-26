# Heartbeats/EnergyMon Profiling Tool

This project provides a performance/power profiler using the
`heartbeats-simple` and `energymon` projects.

Find the latest `heartbeats-simple` libraries at
[https://github.com/libheartbeats/heartbeats-simple](https://github.com/libheartbeats/heartbeats-simple).

Find the latest `energymon` libraries at
[https://github.com/energymon/energymon](https://github.com/energymon/energymon).

## Dependencies

The libraries `hbs-pow` from `heartbeats-simple` and `energymon-default` from
`energymon` must be installed to the system.

The `pkg-config` utility is required during build to locate these dependencies.

## Building

This project uses CMake.

To build all libraries, run:

``` sh
mkdir _build
cd _build
cmake ..
make
```

To build static libraries instead of shared objects, turn off `BUILD_SHARED_LIBS` when running `cmake`:

``` sh
cmake .. -DBUILD_SHARED_LIBS=false
```

### Installing

To install all libraries and headers, run with proper privileges:

``` sh
make install
```

On Linux, installation typically places libraries in `/usr/local/lib` and
header files in `/usr/local/include/he-profiler`.

### Uninstalling

Install must be run before uninstalling in order to have a manifest.

To remove libraries and headers installed to the system, run with proper
privileges:

``` sh
make uninstall
```
## Usage

The most straightforward approach is to use the macros defined in `he-profiler.h`.
By default, profiling is disabled with no performance or memory overhead.
To enable profiling, define `HE_PROFILER_ENABLE` at compile time (either in CFLAGS or in source before including the header file).
You will need to link with the `he-profiler` library.

If you choose to use the functions directly instead of the macros, you can disable profiling by linking with the `he-profiler-dummy` library instead.
You will be responsible for managing the `he_profiler_event` structs.

You should create an enum of the different event types that you will profile.
If you want a background profiler to continually track application power usage (recommended), include an `APPLICATION` profiler.

### Initialization

Before using the profiler, you must initialize it with the `HE_PROFILER_INIT` macro (`he_profiler_init` function).
This will open necessary files and start the background application profiler (if specified).
The parameters for this macro/function are:

* `num_profilers`: The total number of profiler event types.
 This value must be > 0.
* `profiler_names`: The profiler names.
 Log files will use these names in the form `heartbeat-<profiler_name>.log`.
 To disable file logging for a profiler, use NULL at its index in the array.
 A `NULL` value for this parameter disables all file logging.
* `window_sizes`: The number of events that define a sliding window period for each profiler.
 These are also number of events between writing to each profiler's log file.
 A `NULL` for this parameter or array entries that are 0 will use the `default_window_size`.
* `default_window_size`: The default number of events for a window period.
* `app_profiler_id`: The identifier (enum) of the background `APPLICATION` profiler.
 To disable, specify a value >= `num_profilers`.
 This profiler polls the `energymon` implementation at regular intervals.
 Applications should not use this profiler themselves.
* `app_profiler_min_sleep_us`: The minimum number of microseconds that the `APPLICATION` profiler should sleep for.
 By default it will poll at the `energymon` implementation's update interval, but never more than every 10 milliseconds (100 reads/second).
 Use 0 for the default.
* `log_path`: The directory to store log files in.
 A NULL value defaults to the working directory.

### Profiling Events

If using the macros, you have two options for starting an event.
The most straightforward approach is to call `HE_PROFILER_EVENT_BEGIN` with a unique name.
This will create the event on the stack in the current scope.
If the current scope is insufficient, you are responsible for managing the `he_profiler_event` struct.
Use the reentrant macro `HE_PROFILER_EVENT_BEGIN_R` with the name of the predeclared struct (not a pointer!).
If you are not using the macros, use the `he_profiler_event_begin` function.

When an event is finished, call the `HE_PROFILER_EVENT_END` macro (`he_profiler_event_end` function).
If another event begins immediately (not necessarily the same event type), you may reuse the event struct and call `HE_PROFILER_EVENT_END_BEGIN` (`he_profiler_event_end_begin`).
These two macros/functions take the following parameters:

* `event`: The event (the name if using the macros, a pointer to the struct if using the functions).
* `profiler`: The event type identifier (enum).
* `id`: An identifier for this particular event - preferably unique, but not required.
* `work`: The number of work units completed by the event - usually just 1.

### Cleanup

You must clean up when you are finished by calling the `HE_PROFILER_FINISH` macro (`he_profiler_finish` function).
This stops the `APPLICATION` profiler, flushes the remaining log data to files, and frees resources.
Failure to clean up may result in losing profiling data.
