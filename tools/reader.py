#!/usr/bin/env python
# encoding: utf-8
import glob, sys
import pandas as pd
import numpy as np

class TraceReader:
    def __init__(self, path):
        self.path = path                                # path to the log files
        self.start_time = 0
        self.end_time = 0
        self.procs = len(glob.glob(path+"/*.itf"))      # total number of processes
        self.data = None                                # trace log for each rank
        self.meta = []                                  # meatadata for each rank
        self.files = set()                              # All the files ever accessed
        for r in range(self.procs):
            self.meta.append( self.get_metadata(r) )
            for record in self.get_metadata(r):
                self.files.add(record.split(" ")[0])
        self.files = sorted(list(self.files))

    def get_metadata(self, rank):
        if len(self.meta) > rank:
            return self.meta[rank]
        lines = None
        with open(self.path+"/"+str(rank)+".mt", 'r') as f:
            lines = f.readlines()
        lines = list( map(str.strip, lines) )       # remove the trailing newline
        return lines

    def get_data(self, rank):
        lines = None
        with open(self.path+"/"+str(rank)+".itf", 'r') as f:
            lines = f.readlines()
        from operator import itemgetter
        lines = sorted(lines, key=itemgetter(0))    # sort by timestemp
        lines = list( map(str.strip, lines) )       # remove the trailing newline
        return lines

    def get_posix_io(self):
        if self.data is None:
            self.data, self.start_time, self.end_time = read_traces(self.path)
            return self.data
        return self.data



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
    start_time, end_time = 10e12, 0

    log_files = glob.glob(path+"/*.itf")

    for path in log_files:
        rank = int( path.split("/")[-1].split(".")[0] )

        offsets = {}    # offsets[filename] keeps current offset

        with open(path, 'r') as f:
            lines = f.readlines()
            lines = sort_records(lines)
            start_time = min(start_time, float(lines[0].split(" ")[0]))
            end_time = max(end_time, float(lines[-1].split(" ")[0]))
            for line in lines:
                fields = line.split(" ")
                timestamp = float(fields[0])
                duration = float(fields[-1])
                func = fields[1]
                parms = ("".join(fields[2:-1]))[1:-1]
                parms = parms.split(",")
                whence = -1     # don't change offset by default

                # Get the filename, count for every funciton call
                if func == "close" or func == "fclose" or func == "fclose64" or func == "close64" \
                    or func == "open" or func == "fopen" or func == "fopen64" or func == "open64":
                    filename = parms[0]
                    count = 0
                    whence = 0
                    offset = 0 if filename not in offsets else offsets[filename]
                elif func == "fwrite" or func == "fread":
                    filename = parms[3]
                    count = int(parms[1]) * int(parms[2])
                    whence = 1
                    offset = 0 if filename not in offsets else offsets[filename]
                elif func == "read" or func == "write":
                    filename, count = parms[0], int(parms[2])
                    whence = 1
                    offset = 0 if filename not in offsets else offsets[filename]
                elif func == "pwrite" or func == "pwrite64" or func == "pread" or func == "pread64":
                    filename, count, offset = parms[0], int(parms[2]), int(parms[3])
                elif func == "writev" or func == "readv":
                    filename = parms[0]
                    count = int(parms[1].split(":[")[1].split("]")[0])
                    offset = 0 if filename not in offsets else offsets[filename]
                elif func == "lseek" or func == "lseek64":
                    filename = parms[0]
                    offset = int(parms[1])
                    count, whence = int(parms[1]), int(parms[2])
                else:
                    continue

                ops.append([timestamp, duration, rank, func, filename, offset, count])
                update_offset(offsets, filename, count, whence)

    df = create_dataframe(ops)
    df['timestamp'] -= start_time
    return df, start_time, end_time
