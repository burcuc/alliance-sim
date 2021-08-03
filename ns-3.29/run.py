#!/usr/bin/env python
import os
import sys
import subprocess
import argparse
import time

def run_experiment(experiment_file, N, arguments, output_file):
    command = []
    command.append("./waf")
    command.append("--run")
    arguments = [" --" + arg.strip() for arg in arguments.split(',')] if(arguments) else []
    results = " --results=" + Args.results_dir if(Args.results_dir) else ""
    command.append("scratch/" + experiment_file + results + " --N=" + N  +  "".join(arguments))
    print(command)
    process = subprocess.Popen(command, stdout=output, stderr=output)
    time.sleep(3)
    return process


parser = argparse.ArgumentParser()
parser.add_argument("-e", "--experiment_file",required=True, help="experiment to run")
parser.add_argument("-n", "--experiment_name", help="experiment name")
parser.add_argument("-o", "--results_dir", help="directory to store the results")
parser.add_argument("-r", "--number_of_repeats", default=1, type=int, help="number of times to repeat the experiment")
parser.add_argument("-s", "--experiment_sizes", required=True, help="number of nodes for different experiments, use format: 8,16,32")
parser.add_argument("-args", "--arguments", help="extra arguments for ns3, use format: comp_factor=2,cluster=2")
Args = parser.parse_args()

sizes = Args.experiment_sizes.split(',')

if(not Args.experiment_name):
    Args.experiment_name = Args.experiment_file

if(not Args.results_dir):
    Args.console = True
else:
    Args.console = False
    os.system("mkdir -p " + Args.results_dir + "/" + Args.experiment_name)

for i in range(Args.number_of_repeats):
    print("starting set " + str(i) + " of experiments...")
    processes = []
    for size in sizes:
        if(not Args.console):
            output_dir = Args.results_dir + "/" + Args.experiment_name + "/" + size
            os.system("mkdir -p " + output_dir)
            experiment_no = str(len(os.listdir(output_dir)))    
            output = open(output_dir + "/" + experiment_no, "w")
        else:
            output = 1
        processes.append(run_experiment(Args.experiment_file, size, Args.arguments, output))
    print("waiting for set of experiments to complete...")
    for process in processes:
        process.wait()