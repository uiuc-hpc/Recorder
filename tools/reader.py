#!/usr/bin/env python
# encoding: utf-8
import glob, sys
import pandas as pd
import numpy as np

# Sort the records by the timestamp field
def sort_records(lines, by=0):
    from operator import itemgetter
    return sorted(lines, key=itemgetter(by))

'''
Create a Pandas DataFrame
The input must have the following fomrat:
    ops is a list of list, where each entry is:
        [ timestamp, duration, rank, func, filename, offset, count]

'''
def create_dataframe(ops):
    df = pd.DataFrame(ops, columns =['timestamp', 'duration', 'rank', 'func', 'filename', 'offset', 'count'])
    return df


def update_offset(offsets, filename, count, whence):
    if filename not in offsets:
        offsets[filename] = 0
    if whence == 0:         # SEEK_SET, set to offset
        offsets[filename] = count
    elif whence == 1:       # SEEK_CUR, current + offset
        offsets[filename] += count


'''
This is the only funciton that will be called from outside
This returns a Pandas.DataFrame that contains all information
Each record has the following columns (see create_datafram())
['timestamp', 'duration', 'rank', 'func', 'filename', 'offset', 'count']
'''
def read_traces(path):
    ops = []

    log_files = glob.glob(path+"/*.itf")

    for path in log_files:
        rank = int( path.split("/")[-1].split(".")[0] )

        offsets = {}    # offsets[filename] keeps current offset

        with open(path, 'r') as f:
            lines = f.readlines()
            lines = sort_records(lines)
            for line in lines:
                fields = line.split(" ")
                timestamp = float(fields[0])
                duration = float(fields[-1])
                func = fields[1]
                parms = ("".join(fields[2:-1]))[1:-1]
                parms = parms.split(",")

                whence = -1     # don't change offset

                # Get the filename, count for every funciton call
                if func == "fwrite" or func == "fread":
                    filename = parms[3]
                    count = int(parms[1]) * int(parms[2])
                    whence = 1
                elif func == "read" or func == "write":
                    filename, count = parms[0], int(parms[2])
                    whence = 1
                elif func == "pwrite" or func == "pwrite64":
                    filename, count = parms[0], int(parms[2])
                elif func == "writev" or func == "readv":
                    filename = parms[0]
                    count = int(parms[1].split(":[")[1].split("]")[0])
                elif func == "lseek" or func == "lseek64":
                    filename = parms[0]
                    count, whence = int(parms[1]), int(parms[2])
                else:
                    continue

                offset = 0 if filename not in offsets else offsets[filename]
                ops.append([timestamp, duration, rank, func, filename, offset, count])
                update_offset(offsets, filename, count, whence)

    df = create_dataframe(ops)
    min_time =  df['timestamp'].min()
    df['timestamp'] -= min_time

