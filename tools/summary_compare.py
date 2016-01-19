#!/usr/bin/env python

import argparse
import matplotlib.pyplot as plt
import numpy as np
import os
from os import path
import sys

DEFAULT_HEARTBEAT_DIR = "heartbeat_logs"
DEFAULT_OUTPUT_PNG = "summary_comparison.png"
DEFAULT_SUMMARY_FILE = "summary.txt"


def autolabel(rects, ax):
    """Attach some text labels.
    """
    for rect in rects:
        ax.text(rect.get_x() + rect.get_width() / 2., 1.05 * rect.get_height(), '', ha='center', va='bottom')


def plot_raw_totals(plot_data, max_time, max_time_std, max_energy, max_energy_std, output_file):
    """Plot the raw totals.

    Keyword arguments:
    plot_data -- (profiler name, total_time, total_time_std, total_energy, total_energy_std)
    max_time, max_time_std, max_energy, max_energy_std -- single values
    """
    plot_data = sorted(plot_data)
    keys = [p for (p, tt, tts, te, tes) in plot_data]
    total_times = [tt for (p, tt, tts, te, tes) in plot_data]
    total_times_std = [tts for (p, tt, tts, te, tes) in plot_data]
    total_energies = [te for (p, tt, tts, te, tes) in plot_data]
    total_energies_std = [tes for (p, tt, tts, te, tes) in plot_data]

    fig, ax1 = plt.subplots()
    ind = np.arange(len(keys))  # the x locations for the groups
    width = 0.35  # the width of the bars
    # add some text for labels, title and axes ticks
    ax1.set_title('Time/Energy Data')
    ax1.set_xticks(ind + width)
    ax1.set_xticklabels(keys, rotation=45)
    fig.set_tight_layout(True)
    fig.set_size_inches(max(6, len(plot_data)) / 1.5, 8)

    ax2 = ax1.twinx()

    # set time in ms instead of sec
    total_times_std *= np.array(1000.0)
    total_times *= np.array(1000.0)
    total_energies_std /= np.array(1000000.0)
    total_energies /= np.array(1000000.0)
    ax1.set_ylabel('Time (ms)')
    ax2.set_ylabel('Energy (Joules)')

    rects1 = ax1.bar(ind, total_times, width, color='r', yerr=total_times_std)
    rects2 = ax2.bar(ind + width, total_energies, width, color='y', yerr=total_energies_std)
    ax1.legend([rects1[0], rects2[0]], ['Time', 'Energy'])

    # set axis
    x1, x2, y1, y2 = plt.axis()
    ax1.set_ylim(ymin=0, ymax=((max_time + max_time_std) * 1.25 * 1000.0))
    ax2.set_ylim(ymin=0, ymax=((max_energy + max_energy_std) * 1.25 / 1000000.0))

    autolabel(rects1, ax1)
    autolabel(rects2, ax2)

    # plt.show()
    plt.savefig(output_file)
    plt.close(fig)


def create_raw_total_data(data):
    """Get the raw data to plot
    Return: [(config_id, time_mean, time_stddev, energy_mean, energy_stddev)]

    Keyword arguments:
    data -- [(id, [(time, energy)])]
    """
    # We can't assume that the same number of heartbeats are always issued across trials
    # key: config id; value: list of timing sums for each trial
    total_times = {}
    # key: config id; value: list of energy sums for each trial
    total_energies = {}
    for (config_id, config_data) in data:
        for (time, energy) in config_data:
            # add to list to be averaged later
            time_list = total_times.get(config_id, [])
            time_list.append(time)
            total_times[config_id] = time_list
            energy_list = total_energies.get(config_id, [])
            energy_list.append(energy)
            total_energies[config_id] = energy_list

    # Get mean and stddev for time and energy totals
    return [(config_id,
             np.mean(total_times[config_id]),
             np.std(total_times[config_id]),
             np.mean(total_energies[config_id]),
             np.std(total_energies[config_id]))
            for config_id in total_times.keys()]


def plot_comparisons(data, output_file):
    """Plot column charts of the raw total time/energy spent in each configuration.

    Keyword arguments:
    data -- [(id, [(time, energy)])]
    output_file -- the name of the output PNG
    """
    # [(config__id, time_mean, time_stddev, energy_mean, energy_stddev)]
    raw_totals_data = create_raw_total_data(data)

    mean_times = []
    mean_times_std = []
    mean_energies = []
    mean_energies_std = []
    for (c, tt, tts, te, tes) in raw_totals_data:
        mean_times.append(tt)
        mean_times_std.append(tts)
        mean_energies.append(te)
        mean_energies_std.append(tes)
    # get consistent max time/energy values across plots
    max_t = np.max(mean_times)
    max_t_std = np.max(mean_times_std)
    max_e = np.max(mean_energies)
    max_e_std = np.max(mean_energies_std)
    plot_raw_totals(raw_totals_data, max_t, max_t_std, max_e, max_e_std, output_file)


def parse_trial_summary(trial_dir, summary_filename):
    """Parse the summary.txt files
    Returns: (time, energy)

    Keyword arguments:
    trial_dir -- the trial directory
    summary_filename -- the summary text file name
    """
    time_sec = 0
    energy_uj = 0
    with open(path.join(trial_dir, summary_filename)) as summary:
        lines = (line.split(':') for line in summary)
        for line in lines:
            if len(line) >= 2:
                if line[0].startswith("Time (sec)"):
                    time_sec = line[1].strip()
                elif line[0].startswith("Energy (uJ)"):
                    energy_uj = line[1].strip()
    return np.array([time_sec, energy_uj]).astype(np.float)


def parse_all_trials(heartbeats_dir, summary_filename):
    """
    Returns: [(time, energy)]

    Keyword arguments:
    heartbeats_dir -- the actual directory with the trials
    summary_filename -- the summary text file name
    """
    return [parse_trial_summary(path.join(heartbeats_dir, trial_dir), summary_filename)
            for trial_dir in os.listdir(heartbeats_dir)]


def parse_summaries(directories, heartbeat_dir_name, summary_filename):
    """Parse the summary.txt files
    Returns: [(id, [(time, energy)])]

    Keyword arguments:
    directories -- list of directories to search
    heartbeat_dir_name -- the name of the directory with trials
    summary_filename -- the summary text file name
    """
    return [(d, parse_all_trials(path.join(d, heartbeat_dir_name), summary_filename)) for d in directories]


def main():
    """This script processes summary log files and produces visualizations to compare total time and energy.
    """
    # Parsing the input of the script
    parser = argparse.ArgumentParser(description="Process summary log files")
    parser.add_argument("-d", "--directories",
                        required=True, nargs='+',
                        help="Directories that contain 'heartbeat_logs' subdirectories, for example \
                        \"-d config1 config2 config3\"")
    parser.add_argument("-hd", "--heartbeat-dir",
                        default=DEFAULT_HEARTBEAT_DIR,
                        help="Specify the application's heartbeat log directory, for example \"-h heartbeat_logs\"")
    parser.add_argument("-o", "--output",
                        default=DEFAULT_OUTPUT_PNG,
                        help="Specify the log output file, for example \"-o summary_comparison.png\"")
    parser.add_argument("-s", "--summary-file",
                        default=DEFAULT_SUMMARY_FILE,
                        help="Specify the summary file name, for example \"-s summary.txt\"")

    args = parser.parse_args()
    directories = args.directories
    heartbeat_dir = args.heartbeat_dir
    output_file = args.output
    summary_file = args.summary_file

    data = parse_summaries(directories, heartbeat_dir, summary_file)
    # TODO: Plot time/energy column charts and print average power on top of the columns
    plot_comparisons(data, output_file)

if __name__ == "__main__":
    main()
