#!/usr/bin/env python
# encoding: utf-8

import os, sys
from prettytable import PrettyTable
from html_writer import HTMLWriter
from ctypes import *

libreader = CDLL("./libreader.so")


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
                ("filemap", c_char_pp)]

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
    print(bwTable)


# 1. Number of file accessed per rank
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
    print(fileTable)
    html.fileTable = fileTable.get_html_string()

def file_open_flags(records, localDefs):

    # key: filename, value: open flags set
    openFlags = {}

    libreader.get_open_flag.argtypes = [Record, c_char_p]

    flag = create_string_buffer(128)
    filename = create_string_buffer(128)

    for rank in range(len(localDefs)):
        for i in range(localDefs[rank].total_records):
            res = libreader.get_open_flag(records[rank][i], filename, flag)
            if res == 1:
                if filename.value not in openFlags:
                    openFlags[filename.value] = set([flag.value])
                else:
                    openFlags[filename.value].add(flag.value)
    print(openFlags)


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


    OUTPUT_DIR = "./reports.out"
    # Create output directory
    os.system("mkdir -p "+OUTPUT_DIR+"/figures")
    html = HTMLWriter("./simple_report.html")

    # 0. Bandwidth info
    bandwidth_info(records, localDefs)

    # 1. File statistics
    file_statistics(localDefs, html)

    file_open_flags(records, localDefs)


