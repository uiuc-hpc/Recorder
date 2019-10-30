#!/usr/bin/env python
# encoding: utf-8
import sys
import struct
import ctypes

def read_metadata(path):
    filename_id_map = {};
    f = open(path, 'r')
    for line in f.readlines():
        filename, file_id, file_size =  line.split(" ")
        filename_id_map[int(file_id)] = filename
    f.close()
    return filename_id_map

if __name__ == "__main__":

    recorder = ctypes.cdll.LoadLibrary("../lib/librecorder.so")
    recorder.get_function_name_by_id.argtypes = [ctypes.c_int]
    recorder.get_function_name_by_id.restype = ctypes.c_char_p

    logfile_path = sys.argv[1] + "0.itf"
    metafile_path = sys.argv[1] + "0.mt"

    read_metadata(metafile_path)

    output = open(logfile_path+".txt", "w")
    lines = []
    with open(logfile_path, 'rb') as f:
        lines = f.readlines()
        for idx, line in enumerate(lines):
            status = struct.unpack("c", line[0:1])[0]
            tstart, tend = struct.unpack("ii", line[1:9])
            func_id =  struct.unpack("B", line[9:10])[0]

            args = line[11:-1].split(" ")

            outline = "%d %d %s %s\n" %(tstart, tend,   \
                    recorder.get_function_name_by_id(func_id), " ".join(args))
            print(outline[0:-1])

            output.write(outline)

    output.close()
