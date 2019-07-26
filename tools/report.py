#!/usr/bin/env python
# encoding: utf-8

'''
The report contains the following part:
1. Number of files
2. A full file list with its size
3. List of function calls and how many times a function was called
'''
import glob, sys, os
from reader import TraceReader
from prettytable import PrettyTable
from html_writer import HTMLWriter
from vis import draw_bar_chart, draw_pie_chart, draw_hist_chart
from vis import draw_overall_time_chart, draw_offset_vs_rank, draw_offset_vs_time

OUTPUT_DIR = os.getcwd() + "/reports.out/"

def file_statistics(tr: TraceReader, html:HTMLWriter):
    # 1. Number of file accessed by rank
    fileTable = PrettyTable()
    header = ["", "Total"]
    for r in range(tr.procs): header.append("Rank "+str(r))
    fileTable.field_names = header
    row = ["Number of file accessed", len(tr.files)]

    fileSizes = {}
    for rank in range(tr.procs):
        fileSet = set()
        lines = tr.get_metadata(rank)
        for record in lines:
            tmp = record.split(" ")
            filename = tmp[0].split('/')[-1]        # filename
            if filename not in fileSizes:
                fileSizes[filename] = int(tmp[2])       # filesize
            else:                                   # not all ranks received the same correct file size from stat command
                fileSizes[filename] = max(fileSizes[filename], int(tmp[2]))
            fileSet.add(filename)
        row.append(len(fileSet))                    # number of files accessed by rank

    fileTable.add_row(row)
    print(fileTable)
    html.fileTable = fileTable.get_html_string()

    # 2. File access mode
    accessModeTable = PrettyTable()
    accessModeTable.field_names = ["File", "Read Only", "Write Only", "Read & Write"]
    df = tr.get_posix_io()
    for name in tr.files:
        write_count = df[(df['filename'] == name) & (df['func'].str.contains("write"))].shape[0]
        read_count = df[(df['filename'] == name) & (df['func'].str.contains("read"))].shape[0]
        write_only, read_only = write_count > 0 and read_count == 0, read_count > 0 and write_count == 0
        read_write = False if (read_count == 0 and write_count == 0) else ((not read_only) and (not write_only))
        accessModeTable.add_row([name.split("/")[-1], read_only, write_only, read_write])
        html.fileAccessModeTable = accessModeTable.get_html_string()
    print(accessModeTable)


    # 3. File sizes image
    x, y = [], []
    for name in sorted(fileSizes.keys()):
        x.append(name)
        y.append(fileSizes[name]/1024.0)
    draw_bar_chart(x, y, title="File Size (KBytes)", save_to=OUTPUT_DIR+html.fileSizeImage, horizontal=True, logScale=True, xlabel="File size (KBytes)")


def function_statistics(tr: TraceReader, html:HTMLWriter):
    functionCounter = {}
    for rank in range(tr.procs):
        lines = tr.get_data(rank)
        for record in lines:
            func = record.split(" ")[1]
            if func not in functionCounter:
                functionCounter[func] = [0] * tr.procs
            functionCounter[func][rank] += 1

    # Function count by name
    table = PrettyTable()
    header = ["Function name"]
    for r in range(tr.procs): header.append("Rank "+str(r))
    table.field_names = header
    for func in functionCounter:
        table.add_row([func] + functionCounter[func])
    print(table)
    html.functionTable = table.get_html_string()

    # Function count by type (MPI/POSIX/HDF5) - pie chart
    mpi, posix, hdf5 = 0, 0, 0
    for func in functionCounter:
        if func.startswith("MPI"): mpi += sum(functionCounter[func])
        elif func.startswith("H5"): hdf5 += sum(functionCounter[func])
        else: posix += sum(functionCounter[func])
    draw_pie_chart(["MPI", "POSIX", "HDF5"], [mpi, posix, hdf5], save_to=OUTPUT_DIR+html.functionCountImage)

    # Function count by sequential/consective
    sequential, consecutive, random = 0, 0, 0
    for filename in tr.files:
        df = tr.get_posix_io()
        df = df[tr.get_posix_io()['filename'] == filename]
        if df.shape[0] == 0: continue
        last_op = df.iloc[0]
        for index, op in df.iloc[1:].iterrows():
            if ( op['offset'] == last_op['offset'] + last_op['count'] ):
                consecutive += 1
            elif ( op['offset'] > last_op['offset'] + last_op['count'] ):
                sequential += 1
            else:
                random += 1
            last_op = op
    draw_pie_chart(["Sequential", "Consecutive", "Random"], [sequential, consecutive, random], save_to=OUTPUT_DIR+html.functionAccessTypeImage)

    # Cumulative I/O access sizes
    df = tr.get_posix_io()
    write_sizes = df['count'][df['func'].str.contains('write')]
    read_sizes = df['count'][df['func'].str.contains('read')]
    draw_hist_chart([write_sizes, read_sizes], nbins=50, labels=["write", "read"],
            title="Cumulative percentage of I/O access sizes", xlabel="I/O size (Bytes)", ylabel="Percentage", save_to=OUTPUT_DIR+html.ioSizesImage)


def offset_statistics(tr: TraceReader, html: HTMLWriter):
    # 1. Overall time image
    draw_overall_time_chart(tr, save_to=OUTPUT_DIR+html.overallTimeChart)

    # 1. Offset vs Ranks image
    draw_offset_vs_rank(tr, save_to =OUTPUT_DIR+html.offsetVsRankImage)

    draw_offset_vs_time(tr, save_to=OUTPUT_DIR+html.offsetVsTimeImage)

    # 2. Access pattern table, use the sorting algorithm to find the interleave intervals
    # Complexity: O(nlogn) * number of files
    accessPatternTable = PrettyTable()
    accessPatternTable.field_names = ["File", "R->R (self)", "R->R (others)", "R->W (self)", "R->W (others)", \
                                    "W->R (self)", "W->R (others)", "W->W (self)", "W->W (others)"]
    df = tr.get_posix_io()
    for filename in tr.files:
        mask = (df['filename'] == filename) & ( (df['func'].str.contains("read")) |  (df['func'].str.contains("write")) )
        records = df[mask][['timestamp', 'offset','count','func','rank']].values.tolist()
        from operator import itemgetter
        records = sorted(records, key=itemgetter(1))    # sort by starting position

        # now find ovelapping intervals
        self_rr, self_rw, self_wr, self_ww = False, False, False, False
        other_rr, other_rw, other_wr, other_ww = False, False, False, False
        for i in range(len(records)-1):
            op1, op2 = records[i], records[i+1]
            if op2[1] < op1[1]+op1[2]:     # found it !
                if op1[4] == op2[4]:    # self
                    if "read" in op1[3] and "read" in op2[3]: self_rr = True
                    if "write" in op1[3] and "write" in op2[3]: self_ww = True
                    if "read" in op1[3] and "write" in op2[3]:
                        if op1[0] < op2[0]: self_rw = True
                        else: self_wr = True
                    if "write" in op1[3] and "read" in op2[3]:
                        if op1[0] < op2[0]: self_wr = True
                        else: self_rw = True
                else:                   # other
                    if "read" in op1[3] and "read" in op2[3]: other_rr = True
                    if "write" in op1[3] and "write" in op2[3]: other_ww = True
                    if "read" in op1[3] and "write" in op2[3]:
                        if op1[0] < op2[0]: other_rw = True
                        else: other_wr = True
                    if "write" in op1[3] and "read" in op2[3]:
                        if op1[0] < op2[0]: other_wr = True
                        else: other_rw = True
        accessPatternTable.add_row([filename.split("/")[-1], self_rr, other_rr, self_rw, other_rw, self_wr, other_wr, self_ww, other_ww])
    print(accessPatternTable)
    html.accessPatternTable = accessPatternTable.get_html_string()

if __name__ == "__main__":
    # 1. Create output directory
    os.system("mkdir -p "+OUTPUT_DIR+"/figures")

    html = HTMLWriter("./simple_report.html")
    tr = TraceReader(sys.argv[1])
    file_statistics(tr, html)
    function_statistics(tr, html)
    offset_statistics(tr, html)
    html.write_html()
