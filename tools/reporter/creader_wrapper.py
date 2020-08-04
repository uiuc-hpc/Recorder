#!/usr/bin/env python
# encoding: utf-8
from ctypes import *
import sys

class LocalMetadata(Structure):
    _fields_ = [
            ("start_timestamp", c_double),
            ("end_timestamp", c_double),
            ("num_files", c_int),
            ("total_records", c_int),
            ("filemap", POINTER(c_char_p)),
            ("file_sizes", POINTER(c_size_t)),
            ("function_count", c_int*256),
    ]

    # In Python3, self.filemap[i] is 'bytes' type
    # For compatable reason, we convert it to str type
    # and will only use self.filenames[i] to access the filename
    def filemap_to_strs(self):
        self.filenames = [''] * self.num_files
        for i in range(self.num_files):
            if type(self.filemap[i]) != str:
                self.filenames[i] = self.filemap[i].decode('utf-8')
            else:
                self.filenames[i] = self.filemap[i]

class GlobalMetadata(Structure):
    _fields_ = [
            ("time_resolution", c_double),
            ("total_ranks", c_int),
            ("compression_mode", c_int),
            ("peephole_window_size", c_int),
    ]


class Record(Structure):
    _fields_ = [
            ("status", c_char),
            ("tstart", c_double),
            ("tend", c_double),
            ("func_id", c_ubyte),
            ("arg_count", c_int),
            ("args", POINTER(c_char_p)),    # Note in python3, args[i] is 'bytes' type
            ("res", c_int),
    ]

    # In Python3, self.args[i] is 'bytes' type
    # For compatable reason, we convert it to str type
    # and will only use self.arg_strs[i] to access the filename
    def args_to_strs(self):
        arg_strs = [''] * self.arg_count
        for i in range(self.arg_count):
            if(type(self.args[i]) == str):
                arg_strs[i] = self.args[i]
            else:
                arg_strs[i] = self.args[i].decode('utf-8')
        return arg_strs

'''
GM: Global Metadata
LMs: List of Local Metadata
records: List (# ranks) of Record*, each entry (Record*) is a list of records for that rank
'''
class RecorderReader:
    def str2char_p(self, s):
        return c_char_p( s.encode('utf-8') )

    def __init__(self, logs_dir):
        libreader = cdll.LoadLibrary("../C/reader.so")
        libreader.read_records.restype = POINTER(Record)

        self.GM = GlobalMetadata()
        self.LMs = []
        self.records = []

        libreader.read_global_metadata(self.str2char_p(logs_dir + "/recorder.mt"), pointer(self.GM))
        self.load_func_list(logs_dir + "/recorder.mt")

        for rank in range(self.GM.total_ranks):
            print("Read trace file for rank: " + str(rank))
            LM = LocalMetadata()
            libreader.read_local_metadata(self.str2char_p(logs_dir+"/"+str(rank)+".mt"), pointer(LM))
            LM.filemap_to_strs()
            self.LMs.append(LM)


            local_records  = libreader.read_records(self.str2char_p(logs_dir+"/"+str(rank)+".itf"), LM.total_records, pointer(self.GM))
            libreader.decompress_records(local_records, LM.total_records)

            self.records.append(local_records)


    def load_func_list(self, global_metadata_path):
        with open(global_metadata_path, 'rb') as f:
            f.seek(24, 0)
            self.funcs = f.read().splitlines()
            self.funcs = [func.decode('utf-8').replace("PMPI", "MPI") for func in self.funcs]


if __name__ == "__main__":
    reader = RecorderReader(sys.argv[1])
