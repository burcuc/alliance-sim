#!/usr/bin/env python
import os
import sys
import argparse
import matplotlib
#matplotlib.use('Agg')
import matplotlib.pyplot as plt
from collections import OrderedDict

def get_data_points(data,size):
    data_points = []
    for run in data[size]:
        if(len(run) == 0):
            continue
        sums = [.0 for i in range(len(run[0]) - 1)]
        no_runs = max(5, int(len(run)/2))
        print(str(size) + " " + str(no_runs))
        for p in range(len(run) - no_runs, len(run)):
            start_time = float(run[p][0].lstrip("+").rstrip("ns"))/1000000000
            for i in range(1,len(run[p])):
                time = float(run[p][i].lstrip("+").rstrip("ns"))/1000000000
                sums[i-1] += time-start_time
        for i in range(len(sums)):
            sums[i] /= no_runs
        data_points.append(sums)
    if(len(data_points) == 0):
        return []
    Args.len = len(data_points[0])
    return data_points

def get_averages(data_points):
    Args.len = len(data_points[0]) 
    sums = [.0 for i in range(Args.len)]
    for data_point in data_points:
        for i in range(Args.len):
            sums[i] += data_point[i]
    for i in range(Args.len):
        sums[i] /= len(data_points)
    return sums

def plot_dist(data, size, show, latex):
    plt.figure(Args.fig)
    Args.fig += 1
    if(latex):
        latex_coordinates = ""
    for run in data[size]:
        end_times = []
        for data_point in run:
            end_time = float(data_point[len(data_point) - 1].lstrip("+").rstrip("ns"))/1000000000
            start_time = float(data_point[0].lstrip("+").rstrip("ns"))/1000000000
            end_times.append(end_time-start_time)
        plt.plot(end_times, marker="o")
        plt.xlabel("number of runs")
        plt.ylabel("latency (s)")
        plt.title(sizes_loc[size] + "/" + str(size))
        if(latex):
            latex_coordinates += "coordinates{"
            for i in range(len(end_times)):
                latex_coordinates += "(" + str(i) + "," + str(end_times[i]) + ")"
            latex_coordinates += "};\n"
    if(latex):
        print(size)
        print(latex_coordinates)
    if(show):
        plt.show()

def plot_avg(data, sizes, show, latex):
    plt.figure(Args.fig)
    Args.fig += 1
    x = []
    y = []
    x_avg = []
    y_avg = [[] for i in range(Args.len)]
    y_plus_error = []
    y_minus_error = []

    for size in sizes:    
        data_points = get_data_points(data, size)
        if(len(data_points) == 0):
            continue
        for data_point in data_points:
            x.append([size for i in range(len(data_point))])
            y.append(data_point)
        x_avg.append(size)
        avgs = get_averages(data_points)
        for i in range(len(avgs)):
            y_avg[i].append(avgs[i])
        min_dp = 1000000000000000
        max_dp = -1
        for data_point in data_points:  
            if data_point[-1] > max_dp:
                max_dp = data_point[-1]
            if data_point[-1] < min_dp:
                min_dp = data_point[-1]
        y_plus_error.append(max_dp - y_avg[len(avgs) - 1][-1])
        y_minus_error.append(y_avg[len(avgs) - 1][-1] - min_dp)
   
    plt.scatter(x, y, marker="o")
    for i in range(Args.len):
        plt.plot(x_avg, y_avg[i], marker="x")
    plt.xlabel("number of nodes")
    plt.ylabel("latency (s)")
    plt.title(Args.results)
    if(latex):
        latex_coordinates = "coordinates{"
        for i in range(len(x_avg)):
            latex_coordinates += "(" + str(x_avg[i]) + "," + str(y_avg[len(avgs) - 1][i]) + ")" + " += (0, " + str(y_plus_error[i]) + ") -= (0, " + str(y_minus_error[i]) + ")"
        latex_coordinates += "}"
        print(latex_coordinates)
    if(show):
        plt.show()

def parse_data(file, data, size):
    data[size] = []
    for line in file:
        if("RUN" in line or "new" in line):
            data[size].append([])
        elif(line.strip()):
            pt = line.strip(',\n').split(',')
            if len(pt) > 1:
                data[size][len(data[size]) - 1].append(pt)

parser = argparse.ArgumentParser()
parser.add_argument("-r", "--results", nargs = "+", required=True, help="results directory of the experiment")
parser.add_argument("-o", "--output", help="output file")
parser.add_argument("--plot_dist", action="store_true")
parser.add_argument("--plot_avg", action="store_true")
parser.add_argument("--latex", action="store_true")
Args = parser.parse_args()

Args.len = 3
Args.fig = 0

sizes = set()
sizes_loc = {}
data = {}

for results_dir in Args.results:
    for d in os.listdir(results_dir):
        if(os.path.isdir(os.path.join(results_dir, d))):
            sizes.add(int(d))
            sizes_loc[int(d)] = results_dir
sizes = sorted(sizes)

for size in sizes:
    data_file = open(os.path.join(sizes_loc[size], str(size), "data"))
    parse_data(data_file, data, size)
    data_file.close()
data = OrderedDict(sorted(data.items(), key = lambda t: t[0]))

for size in sizes:
    if(Args.plot_dist):
        plot_dist(data, size, False,Args.latex)

if(Args.plot_avg):
    plot_avg(data,sizes,False,Args.latex)

plt.show()
