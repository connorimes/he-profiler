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

## Building

This project uses CMake.

To build all libraries, run:

``` sh
mkdir _build
cd _build
cmake ..
make
```

## Installing

To install all libraries and headers, run with proper privileges:

``` sh
make install
```

On Linux, installation typically places libraries in `/usr/local/lib` and
header files in `/usr/local/include/he-profiler`.

## Uninstalling

Install must be run before uninstalling in order to have a manifest.

To remove libraries and headers installed to the system, run with proper
privileges:

``` sh
make uninstall
```
## Usage

The normal use case is to link with the `he-profiler` (or `he-profiler-static`) library at compile time.
If you want to disable profiling, link with the `he-profiler-dummy` (or `he-profiler-dummy-static`) library instead.

You should create an enum of the different event types that you will profile.
If you want a background profiler to continually track application power usage, include an `APPLICATION` profiler.

Before using the profiler, you must initialize it with the `he_profiler_init` function.
This will open necessary files and start a background application profiler if desired.
Parameters for `he_profiler_init` are:

* `num_profilers`: The total number of profiler event types.
 This value must be > 0.
* `application_profiler`: The identifier (enum) of the background `APPLICATION` profiler (-1 to disable).
 This profiler polls the `energymon` implementation at regular intervals.
 Applications should not use this profiler themselves.
* `profiler_names`: The profiler names. Log files will use these names in the form `heartbeat-<profiler_name>.log`.
 To disable file logging for a profiler, use NULL at its index in the array.
 A `NULL` value for this parameter disables all file logging.
* `default_window_size`: The number of events that define a sliding window period.
 This is also number of events between writing to each profiler's log file.
* `env_var_prefix`: Some configurations can be specified by environment variables at runtime - this optional string prefixes those environment variables.
 * `WINDOW_SIZE` - Overrides the `default_window_size` parameter.
 * `MIN_SLEEP_US` - A minimum number of microseconds for the `application_profiler` to sleep between energy readings.
   By default it will poll at the `energymon` implementation's update interval, but never more than every 10 milliseconds (100 reads/second).
* `log_path`: The directory to store log files in.
 A NULL value defaults to the working directory.

You must also clean up when you are finished by calling the `he_profiler_finish` function, which will flush the remaining log data to files and clean up resources.
Failure to clean up may result in losing some profiling data.
This function does not take any parameters.

To profile an event, first create a `he_profiler_event` struct and call `he_profiler_event_begin`.
This function's only parameter is `event` - a pointer to the event struct.
When an event is finished, call the `he_profiler_event_end` function.
If another event begins immediately (not necessarily the same event type), you may reuse the event struct and call `he_profiler_event_end_begin` instead of `he_profiler_event_end` followed by `he_profiler_event_begin`.
These two functions take the following parameters:

* `profiler`: The event type identifier (enum).
* `id`: An identifier for this particular event - preferably unique, but not required.
* `work`: The number of work units completed by the event - usually just 1.
* `event`: A pointer to the event struct passed to `he_profiler_event_begin`.
