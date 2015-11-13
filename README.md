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

## Cleaning

To cleanup build artifacts, run

``` sh
make clean
```
