#!/usr/bin/env python
import os
import sys

def get_time(time):
    time_to_ns = float(time.lstrip("+").rstrip("s"))*1000000000
    return "+" + '{:f}'.format(time_to_ns) + "ns"

def parse_file(file, output):
        first = True
        for line in file:
            if("RUN: " in line):
                time = get_time(line.split()[0])
                if(first):
                    output.write("\nRUN: \n" + time + ',')
                    first = False
                else:
                    output.write(time + '\n' + time + ',')
            elif("all nodes are done" in line):
                time = get_time(line.split()[0])
                output.write(time + '\n')
                
def parse(directory):
    experiments = sorted(os.listdir(directory))
    data = open(os.path.join(directory, "data"), 'w')

    no = 0
    for exp in experiments:
        if("data" in exp):
            continue
        no += 1
        file = open(os.path.join(directory, exp), 'r')
        parse_file(file, data)
        file.close()
    print(no)

root = sys.argv[1]
dirs = [dI for dI in os.listdir(root) if os.path.isdir(os.path.join(root,dI))]
for dir in dirs:
    print(os.path.join(root,dir))
    parse(os.path.join(root,dir))
