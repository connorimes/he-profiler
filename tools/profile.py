#!/usr/bin/env python

import argparse
import datetime
import glob
import os
from os import path
import platform
import shutil
import subprocess
import sys
import time

DEFAULT_GUARD_TIME = 20
DEFAULT_HEARTBEAT_DIR = "."
DEFAULT_OUTPUT_DIR = "."
DEFAULT_SUMMARY_FILE = "summary.txt"
DEFAULT_TRIALS = 1

ENERGY_READER_BIN = "energymon"
ENERGY_READER_TEMP_OUTPUT = "energymon.txt"
STDOUT_FILE = "stdout.txt"
STDERR_FILE = "stderr.txt"


def start_energy_reader(temp_file):
    """Energy reader writes to a file that we will poll.
    """
    try:
        return subprocess.Popen([ENERGY_READER_BIN, temp_file])
    except OSError as e:
        print >> sys.stderr, "Failed to execute '" + ENERGY_READER_BIN + "':", e
        sys.exit(1)


def stop_energy_reader(process, temp_file):
    """Stop the energy reader and remove its temp file.
    """
    try:
        process.kill()
    except OSError as e:
        print >> sys.stderr, "Failed to kill energy reader process with id '" + process.pid + "':", e
    try:
        os.remove(temp_file)
    except OSError as e:
        print >> sys.stderr, "Failed to remove '" + temp_file + "':", e


def read_energy(temp_file):
    """Poll the energy reader's temp file.
    """
    data = 0
    with open(temp_file, "r") as em:
        data = int(em.read().replace('\n', ''))
    return data


def execute(cmd, guard_time, heartbeat_dir, output_dir, summary_file, trial):
    """Run a single execution.
    """
    log_dir = path.join(output_dir, "trial_" + str(trial))
    if os.path.exists(log_dir):
        print "Log directory already exists: " + log_dir
        sys.exit(1)
    os.makedirs(log_dir)

    # Start energy reader
    er_temp_file = path.join(log_dir, ENERGY_READER_TEMP_OUTPUT)
    er_process = start_energy_reader(er_temp_file)

    # Let the system idle before start
    print 'sleep ' + str(guard_time)
    time.sleep(guard_time)

    # Execute
    time_start = time.time()
    energy_start = read_energy(er_temp_file)
    print cmd
    success = True
    try:
        with open(path.join(log_dir, STDOUT_FILE), "wb") as out, \
             open(path.join(log_dir, STDERR_FILE), "wb") as err:
            retcode = subprocess.call(cmd, stdout=out, stderr=err, shell=True)
            if retcode != 0:
                success = False
    except OSError as e:
        print >> sys.stderr, "Failed to execute '" + cmd + "':", e
    energy_end = read_energy(er_temp_file)
    time_end = time.time()
    stop_energy_reader(er_process, er_temp_file)
    if energy_end - energy_start <= 0:
        print "Energy reader failure"
        success = False
    if not success:
        sys.exit(1)

    # Let the system idle after finish
    print 'sleep ' + str(guard_time)
    time.sleep(guard_time)

    for hb_log in glob.glob(heartbeat_dir + "/heartbeat-*.log"):
        if not os.path.isdir(hb_log):
            shutil.move(hb_log, log_dir)

    # Write a file that describes this execution
    uj = energy_end - energy_start
    latency = time_end - time_start
    watts = uj / 1000000.0 / latency
    with open(path.join(log_dir, summary_file), "w") as f:
        f.write("Datetime (UTC): " + datetime.datetime.utcnow().isoformat() + "\n")
        f.write("Platform: " + platform.platform() + "\n")
        f.write("Command: " + cmd + "\n")
        f.write("Trial: " + str(trial) + "\n")
        f.write("Time (sec): " + str(latency) + "\n")
        f.write("Energy (uJ): " + str(uj) + "\n")
        f.write("Power (W): " + str(watts) + "\n")


def main():
    # Parsing the input of the script
    parser = argparse.ArgumentParser(description="Profile timing and energy behavior")
    parser.add_argument("-c", "--command",
                        required=True,
                        help="The command to execute, for example \"-c \"sleep 10\"\"")
    parser.add_argument("-g", "--guard-time",
                        default=DEFAULT_GUARD_TIME, type=int,
                        help="Specify the guard time in seconds around execution, for example \"-g 20\"")
    parser.add_argument("-hd", "--heartbeat-dir",
                        default=DEFAULT_HEARTBEAT_DIR,
                        help="Specify the application's heartbeat log directory, for example \"-h heartbeat_logs\"")
    parser.add_argument("-o", "--output-dir",
                        default=DEFAULT_OUTPUT_DIR,
                        help="Specify the log output directory, for example \"-o logs\"")
    parser.add_argument("-s", "--summary-file",
                        default=DEFAULT_SUMMARY_FILE,
                        help="Specify the summary file name, for example \"-s summary.txt\"")
    parser.add_argument("-t", "--trials",
                        default=DEFAULT_TRIALS, type=int,
                        help="Number of trials to run, for example \"-t 1\"")
    args = parser.parse_args()
    cmd = args.command
    guard_time = args.guard_time
    heartbeat_dir = args.heartbeat_dir
    output_dir = args.output_dir
    summary_file = args.summary_file
    trials = args.trials

    # Create output directory
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Run all trials
    for trial in xrange(1, trials + 1):
        execute(cmd, guard_time, heartbeat_dir, output_dir, summary_file, trial)


if __name__ == "__main__":
    main()
