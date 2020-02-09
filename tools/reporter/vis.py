#!/usr/bin/env python
# encoding: utf-8
import sys, math
import numpy as np
from math import pi
from bokeh.plotting import figure, output_file, show
from bokeh.embed import components
from bokeh.models import FixedTicker
from prettytable import PrettyTable

from reader import RecorderReader, func_list
from html_writer import HTMLWriter

reader = RecorderReader(sys.argv[1])
htmlWriter = HTMLWriter("./")

# 0.0
def record_counts():
    y = []
    for meta in reader.localMetadata:
        y.append(meta.totalRecords)
    x = range(reader.globalMetadata.numRanks)
    p = figure(x_axis_label="Rank", y_axis_label="Number of records", plot_width=400, plot_height=300)
    p.xaxis[0].ticker = FixedTicker(ticks=range(reader.globalMetadata.numRanks))
    p.vbar(x=x, top=y, width=0.6)
    script, div = components(p)
    htmlWriter.recordCount = div+script

# 1.1
def file_counts():
    y = []
    for meta in reader.localMetadata:
        y.append(meta.numFiles)
    x = range(reader.globalMetadata.numRanks)
    p = figure(x_axis_label="Rank", y_axis_label="Number of files accessed", plot_width=400, plot_height=300)
    p.xaxis[0].ticker = FixedTicker(ticks=range(reader.globalMetadata.numRanks))
    p.vbar(x=x, top=y, width=0.6)
    script, div = components(p)
    htmlWriter.fileCount = div+script

# 1.2
def file_access_mode():
    f = {}
    f["O_RDONLY"], f["O_WRONLY"], f["O_RDWR"], f["O_NONBLOCK"], f["O_APPEND"] = 0x000, 0x0001, 0x0002, 0x0004, 0x0008
    f["O_SHLOCK"], f["O_EXLOCK"], f["O_ASYNC"], f["O_FSYNC"] = 0x0010, 0x0020, 0x0040, 0x0080
    f["O_CREAT"], f["O_TRUNC"], f["O_EXCL"] = 0x0200, 0x0400, 0x0800

    # Initialize
    flags_set = {}
    accesses_set = {}
    sizes_set = {}
    for localMetadata in reader.localMetadata:
        for fileInfo in localMetadata.fileMap:
            filename = fileInfo[2]
            flags_set[filename] = set()
            sizes_set[filename] = fileInfo[1]
            accesses_set[filename] = {'read': False, 'write': False}

    def get_flag_str(flag_int):
        flags = ""
        for flag_str in f.keys():
            if flag_int & f[flag_str] > 0: flags += (" | " + flag_str)
        return "O_RDONLY" if flags == "" else flags[3:]


    for rank in range(reader.globalMetadata.numRanks):
        fileMap = reader.localMetadata[rank].fileMap
        for record in reader.records[rank]:
            funcname = func_list[record[3]]
            if "MPI" in funcname or "H5" in funcname: continue

            if "open" in funcname:
                fileId = int(record[4][0])
                filename = fileMap[fileId][2]
                flagStr = ""
                if funcname == "open" or funcname == "open64":
                    flagStr = get_flag_str( int(record[4][1]) )
                elif "fopen" in funcname or "fdopen" in funcname:
                    flagStr = record[4][1]
                else:
                    print("Not regonized: ", funcname)
                flags_set[filename].add(flagStr)

            if "read" in funcname or "write" in funcname :
                fileId = int(record[4][0])
                if "fread" in funcname or "fwrite" in funcname:
                    fileId = int(record[4][3])
                filename = fileMap[fileId][2]
                if "read" in funcname:
                    accesses_set[filename]["read"] = True
                if "write" in funcname:
                    accesses_set[filename]["write"] = True


    table = PrettyTable()
    table.field_names = ['Filename', 'File Size', 'Open Flags', 'Read', 'Write']
    for filename in flags_set:
        table.add_row([filename, sizes_set[filename], list(flags_set[filename]),\
            accesses_set[filename]['read'], accesses_set[filename]['write']])
    htmlWriter.fileAccessModeTable = table.get_html_string()

# 2.1
def function_layers():
    x = {'hdf5':0, 'mpi':0, 'posix':0 }
    for localMetadata in reader.localMetadata:
        for i, count in enumerate(localMetadata.functionCounter):
            if count <= 0: continue
            if "H5" in func_list[i]:
                x['hdf5'] += count
            elif "MPI" in func_list[i]:
                x['mpi'] += count
            else:
                x['posix'] += count

    import pandas as pd
    from bokeh.palettes import Category20c
    data = pd.Series(x).reset_index(name='value').rename(columns={'index':'layer'})
    data['angle'] = data['value']/data['value'].sum() * 2*pi
    data['color'] = Category20c[len(x)]

    from bokeh.transform import cumsum
    p = figure(plot_height=300, plot_width=400)
    p.wedge(x=0, y=1, radius=0.4, start_angle=cumsum('angle', include_zero=True), end_angle=cumsum('angle'),
            line_color="white", fill_color='color', legend_field='layer', source=data)

    p.axis.axis_label=None
    p.axis.visible=False
    p.grid.grid_line_color = None
    script, div = components(p)
    htmlWriter.functionLayers = script+div


# 2.3
def function_counts():
    aggregate = np.zeros(256)
    for localMetadata in reader.localMetadata:
        aggregate += np.array(localMetadata.functionCounter)

    funcnames, counts = np.array([]), np.array([])
    for i in range(len(aggregate)):
        if aggregate[i] > 0:
            funcnames = np.append(funcnames, func_list[i].replace("PMPI", "MPI"))
            counts = np.append(counts, aggregate[i])

    index = np.argsort(counts)[::-1]
    counts = counts[index]
    funcnames = funcnames[index]

    p = figure(x_axis_label="Count", x_axis_type="log", y_axis_label="Function", y_range=funcnames)

    p.hbar(y=funcnames, right=counts, height=0.8, left=1)
    script, div = components(p)
    htmlWriter.functionCount = div + script

# 3.1
def overall_io_activities():

    timeRes = float(reader.globalMetadata.timeResolution)
    nan = float('nan')

    def io_activity(rank):
        x_read, x_write, y_read, y_write = [], [], [], []
        for record in reader.records[rank]:
            funcname = func_list[record[3]]
            if "MPI" in funcname or "H5" in funcname: continue
            if "write" in funcname:
                x_write.append(record[1] * timeRes)
                x_write.append(record[2] * timeRes)
                x_write.append(nan)
            if "read" in funcname:
                x_read.append(record[1] * timeRes)
                x_read.append(record[2] * timeRes)
                x_read.append(nan)

        if(len(x_write)>0): x_write = x_write[0: len(x_write)-1]
        if(len(x_read)>0): x_read = x_read[0: len(x_read)-1]

        y_write = [rank] * len(x_write)
        y_read = [rank] * len(x_read)

        return x_read, x_write, y_read, y_write


    p = figure(x_axis_label="Time", y_axis_label="Rank", plot_width=600, plot_height=400)
    for rank in range(reader.globalMetadata.numRanks):
        x_read, x_write, y_read, y_write = io_activity(rank)
        p.line(x_write, y_write, line_color='red', line_width=20, alpha=1.0, legend_label="write")
        p.line(x_read, y_read, line_color='blue', line_width=20, alpha=1.0, legend_label="read")

    p.yaxis[0].ticker = FixedTicker(ticks=range(reader.globalMetadata.numRanks))
    p.legend.location = "top_left"
    script, div = components(p)
    htmlWriter.overallIOActivities = div + script


if __name__ == "__main__":
    record_counts()

    file_counts()
    file_access_mode()

    function_layers()
    function_counts()

    overall_io_activities()

    htmlWriter.write_html()

