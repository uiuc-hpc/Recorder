#!/usr/bin/env python
# encoding: utf-8

import os, sys
from prettytable import PrettyTable
from html_writer import HTMLWriter
from ctypes import *

from vis import draw_bar_chart, draw_pie_chart, draw_multi_bar_chart

libreader = CDLL("./libreader.so")
OUTPUT_DIR = os.getcwd() + "/reports.out/"


c_char_pp = POINTER(c_char_p)

class Record(Structure):
    _fields_ = [("status", c_char),
                ("tstart", c_double),
                ("tend", c_double),
                ("func_id", c_ubyte),
                ("arg_count", c_int),
                ("args", c_char_pp)]

class RecorderLocalDef(Structure):
    _fields_ = [("start_timestamp", c_double),
                ("end_timestamp", c_double),
                ("num_files", c_int),
                ("total_records", c_int),
                ("filemap", c_char_pp),
                ("file_sizes", POINTER(c_size_t)),
                ("function_count", c_int * 256)]

class RecorderGlobalDef(Structure):
    _fields_ = [("time_resolution", c_double),
                ("total_ranks", c_int),
                ("compression_mode", c_int),
                ("peephole_window_size", c_int)]
class RecorderBandwidthInfo(Structure):
    _fields_ = [("read_size", c_size_t),
                ("write_size", c_size_t),
                ("read_bandwidth", c_double),
                ("write_bandwidth", c_double),
                ("read_cost", c_double),
                ("write_cost", c_double)]

# 0. Bandwidth info
def bandwidth_info(records, localDefs) :
    KB = 1000.0
    bwTable = PrettyTable()
    bwTable.field_names = ["Rank", "Write Size", "Write Bandwidth", "Read Size", "Read Bandwidth"]
    for rank in range( len(records)):
    #for rank in range(1, 2):
        bwinfo = RecorderBandwidthInfo()
        libreader.get_bandwidth_info(records[rank], localDefs[rank], pointer(bwinfo))
        row = [str(rank), "%.3f" %(bwinfo.write_size/KB), "%.3f" %(bwinfo.write_bandwidth/KB),
                          "%.3f" %(bwinfo.read_size/KB), "%.3f" %(bwinfo.read_bandwidth/KB)]
        bwTable.add_row(row)
    html.performanceTable = bwTable.get_html_string()
    print(bwTable)


# 1.1 Number of file accessed per rank
def file_statistics(localDefs, html):
    fileTable = PrettyTable()
    header = ["", "Total"]
    row = ["Number of file accessed", 0]

    fileSet = set([])
    for rank in range(len(localDefs)):
        header.append("Rank "+str(rank))
        row.append(localDefs[rank].num_files)
        for i in range(localDefs[rank].num_files):
            fileSet.add(localDefs[rank].filemap[i])
    row[1] = len(fileSet)   # Set the total number of files across all ranks

    fileTable.field_names = header
    fileTable.add_row(row)
    html.fileTable = fileTable.get_html_string()
    print(fileTable)

# 1.2 File open flags
def file_open_flags(records, localDefs):
    libreader.get_io_filename.restype = c_char_p
    # key: filename, value: {"read": true/false, "write": true/false, "flags:"  open flags set }
    accessMode = {}
    for rank in range(len(localDefs)):
        localDef = localDefs[rank]
        for i in range(localDef.num_files):
            accessMode[localDef.filemap[i]] = {"read": False, "write": False, "flags":set()}

    flag = create_string_buffer(128)
    for rank in range(len(localDefs)):
        localDef = localDefs[rank]
        for i in range(localDef.total_records):
            record = records[rank][i]
            res = libreader.get_open_flag(record, flag)
            if res == 1:    # open ?
                filename = libreader.get_io_filename(byref(record), byref(localDef))
                accessMode[filename]["flags"].add(flag.value)
            else:           # read/write?
                if libreader.is_posix_read_function(byref(record)):
                    filename = libreader.get_io_filename(byref(record), byref(localDef))
                    accessMode[filename]["read"] = True
                elif libreader.is_posix_write_function(byref(record)):
                    filename = libreader.get_io_filename(byref(record), byref(localDef))
                    accessMode[filename]["write"] = True

    fileTable = PrettyTable()
    fileTable.field_names = ["File", "Read Access Mode", "Open Flags"]
    for filename in accessMode:
        filename_str = filename.decode().split("/")[-1]
        row = []
        if accessMode[filename]["read"] and accessMode[filename]["write"]:
            row = [filename_str, "Read & Write", accessMode[filename]["flags"]]
        elif accessMode[filename]["read"]:
            row = [filename_str, "Read only", accessMode[filename]["flags"]]
        elif accessMode[filename]["write"]:
            row = [filename_str, "Write only", accessMode[filename]["flags"]]
        else:
            row = [filename_str, "-", accessMode[filename]["flags"]]
        fileTable.add_row(row)
    html.fileAccessModeTable = fileTable.get_html_string()
    print(fileTable)


# 1.3 File sizes chart
def file_sizes_chart(localDefs, html):
    fileSet = set()
    x, y = [], []
    for rank in range(len(localDefs)):
        for i in range(localDefs[rank].num_files):
            filename = localDefs[rank].filemap[i].decode().split("/")[-1]
            if filename not in fileSet:
                fileSet.add(filename)
                x.append( filename )
                y.append( localDefs[rank].file_sizes[i] / 1024.0 )
    draw_bar_chart(x, y, title="File Size (KBytes)", save_to=OUTPUT_DIR+html.fileSizeImage, horizontal=True, logScale=True, xlabel="File size (KBytes)")

# 2.3 Function Count
def function_count(localDefs, html):
    table = PrettyTable()
    header = ["Function name"]

    for rank in range(len(localDefs)):
        header.append("Rank "+str(rank))
    table.field_names = header

    libreader.get_function_name.restype = c_char_p

    mpi, posix, hdf5 = 0, 0, 0
    for i in range(255):
        counts = []
        for rank in range(len(localDefs)):
            counts.append( localDefs[rank].function_count[i] )

        if max(counts) > 0:
            func = libreader.get_function_name(i).decode()
            # 2.1 Count by type (MPI/HDF5/POSIX)
            if (func.startswith("MPI") or func.startswith("PMPI")): mpi += sum(counts)
            elif func.startswith("H5"): hdf5 += sum(counts)
            else: posix += sum(counts)
            # 2.3 Count by rank
            table.add_row([func]+counts)



    print(table)
    draw_pie_chart(["MPI", "POSIX", "HDF5"], [mpi, posix, hdf5], save_to=OUTPUT_DIR+html.functionCountImage)
    html.functionTable = table.get_html_string()

# 4. IO Sizes chart
def io_sizes_chart(records, localDefs):
    read_count, write_count = {}, {}
    for rank in range(len(localDefs)):
        for i in range(localDefs[rank].total_records):
            record = records[rank][i]
            if libreader.is_posix_read_function(byref(record)):
                size = libreader.get_io_size(record)
                if size not in read_count:
                    read_count[size] = 1
                else:
                    read_count[size] += 1
            if libreader.is_posix_write_function(byref(record)):
                size = libreader.get_io_size(record)
                if size not in write_count:
                    write_count[size] = 1
                else:
                    write_count[size] += 1

    xs, ys = [[], []], [[], []]
    for size in write_count:
        xs[0].append(size)
        ys[0].append(write_count[size])
    for size in read_count:
        xs[1].append(size)
        ys[1].append(read_count[size])
    draw_multi_bar_chart(xs, ys, titles=["Number of writes", "Number of reads"], save_to=OUTPUT_DIR+html.ioSizesImage, logScale=True, xlabel="I/O access sizes (bytes)", ylabel="Function count (log scale)")

if __name__ == "__main__":
    log_path = sys.argv[1] + "/"

    globalDef = RecorderGlobalDef()
    libreader.read_global_metadata((log_path+"recorder.mt").encode("ascii"), pointer(globalDef))

    localDefs = []
    records = []
    for rank in range(globalDef.total_ranks):
        localDef = RecorderLocalDef()
        libreader.read_local_metadata((log_path+str(rank)+".mt").encode("ascii"), pointer(localDef))
        localDefs.append(localDef)

        libreader.read_logfile.restype = POINTER(Record)
        records_this_rank = libreader.read_logfile((log_path+str(rank)+".itf").encode("ascii"), globalDef, localDef)
        libreader.decode(records_this_rank, globalDef, localDef)
        records.append(records_this_rank)

    # Create output directory
    os.system("mkdir -p "+OUTPUT_DIR+"/figures")
    html = HTMLWriter("./simple_report.html")

    # 0. Bandwidth info
    bandwidth_info(records, localDefs)

    # 1.1 File statistics
    file_statistics(localDefs, html)
    # 1.2 File open flags
    file_open_flags(records, localDefs)
    # 1.3
    file_sizes_chart(localDefs, html)
    # 2
    function_count(localDefs, html)
    io_sizes_chart(records, localDefs)

    html.write_html()



