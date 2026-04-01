#!/usr/bin/env python
# encoding: utf-8
from ctypes import *
import sys, os, glob, struct


class VerifyIORecord(Structure):
    # The fields must be identical as PyRecord in tools/reader.h
    _fields_ = [
            ("func_id",    c_int),
            ("call_depth", c_ubyte),
            ("arg_count",  c_ubyte),
            ("args",       POINTER(c_char_p)),    # Note in python3, args[i] is 'bytes' type
    ]

    # In Python3, self.args[i] is 'bytes' type
    # For compatable reason, we convert it to str type
    # and will only use self.arg_strs[i] to access the filename
    """
    def args_to_strs(self):
        arg_strs = [''] * self.arg_count
        for i in range(self.arg_count):
            if(type(self.args[i]) == str):
                arg_strs[i] = self.args[i]
            else:
                arg_strs[i] = self.args[i].decode('utf-8')
        return arg_strs
    """


"""
self.funcs: a list of supported funcitons
self.nprocs
self.num_records[rank] 
self.records[Rank]: per-rank list of VerifyIORecord
"""
class RecorderReader:

    def str2char_p(self, s):
        return c_char_p( s.encode('utf-8') )
    
    def __init__(self, logs_dir):
        if "RECORDER_INSTALL_PATH" not in os.environ:
            msg="Error:\n"\
                "    RECORDER_INSTALL_PATH environment variable is not set.\n" \
                "    Please set it to the path where you installed Recorder."
            print(msg)
            exit(1)

        recorder_install_path = os.path.abspath(os.environ["RECORDER_INSTALL_PATH"])
        libreader_path = recorder_install_path + "/lib/libreader.so"

        if not os.path.isfile(libreader_path):
            msg="Error:\n"\
                "    Could not find Recorder reader library\n"\
                "    Please make sure Recorder is installed at %s",\
                recorder_install_path
            print(msg)
            exit(1);

        # Load function list and the number of processes
        self.logs_dir = logs_dir
        self.__read_num_procs(self.logs_dir + "/recorder.mt")
        self.__load_func_list(self.logs_dir + "/recorder.mt")

        # Set up C reader library
        # Read all VerifyIORecord
        self.libreader = cdll.LoadLibrary(libreader_path)
        self.libreader.recorder_read_verifyio_records.restype = POINTER(POINTER(VerifyIORecord))
        num_records = (c_size_t * self.nprocs)()
        self.records = self.libreader.recorder_read_verifyio_records(self.str2char_p(self.logs_dir), num_records)
        self.num_records = [0 for x in range(self.nprocs)]
        for rank in range(self.nprocs):
            self.num_records[rank] = num_records[rank]

    # Mirror of RecorderMetadata from include/recorder-logger.h (new format).
    # Must stay in sync with the C struct layout.
    class _RecorderMetadata(Structure):
        _fields_ = [
            ("version_major",                    c_int),
            ("version_minor",                    c_int),
            ("version_patch",                    c_int),
            ("total_ranks",                      c_int),
            ("num_funcs",                        c_int),
            ("posix_tracing",                    c_bool),
            ("mpi_tracing",                      c_bool),
            ("mpiio_tracing",                    c_bool),
            ("hdf5_tracing",                     c_bool),
            ("pnetcdf_tracing",                  c_bool),
            ("netcdf_tracing",                   c_bool),
            ("store_tid",                        c_bool),
            ("store_call_depth",                 c_bool),
            ("start_ts",                         c_double),
            ("time_resolution",                  c_double),
            ("ts_buffer_elements",               c_int),
            ("ts_compression",                   c_bool),
            ("interprocess_compression",         c_bool),
            ("interprocess_pattern_recognition", c_bool),
            ("intraprocess_pattern_recognition", c_bool),
        ]

    RECORDER_FUNC_NAME_LEN = 64

    def __read_num_procs(self, metadata_file):
        legacy = os.path.exists(os.path.join(self.logs_dir, "VERSION"))
        with open(metadata_file, 'rb') as f:
            if legacy:
                # old format: total_ranks is the first int
                self.nprocs = struct.unpack('i', f.read(4))[0]
            else:
                # new format: version_major, version_minor, version_patch, total_ranks
                _, _, _, total_ranks = struct.unpack('iiii', f.read(16))
                self.nprocs = total_ranks

    # read supported list of functions from the metadata file
    # invoked in __init__() only
    def __load_func_list(self, metadata_file):
        legacy = os.path.exists(os.path.join(self.logs_dir, "VERSION"))
        with open(metadata_file, 'rb') as f:
            if legacy:
                f.seek(1024, 0)   # skip the reserved metadata block (fixed 1024 bytes)
                self.funcs = f.read().splitlines()
                self.funcs = [func.decode('utf-8') for func in self.funcs]
            else:
                # new format: fixed RECORDER_FUNC_NAME_LEN-byte entries after the struct
                meta_size = sizeof(RecorderReader._RecorderMetadata)
                f.seek(meta_size, 0)
                num_funcs = self.nprocs  # already read; re-read num_funcs from struct
                f.seek(16, 0)            # offset of num_funcs: 4 ints * 4 bytes
                num_funcs = struct.unpack('i', f.read(4))[0]
                f.seek(meta_size, 0)
                self.funcs = []
                for _ in range(num_funcs):
                    raw = f.read(RecorderReader.RECORDER_FUNC_NAME_LEN)
                    name = raw.rstrip(b'\x00').decode('utf-8')
                    self.funcs.append(name)



if __name__ == "__main__":

    import resource, psutil
    print(resource.getrusage(resource.RUSAGE_SELF))
    print('RAM Used (GB):', psutil.virtual_memory()[3]/1000000000)

    reader = RecorderReader(sys.argv[1])

    print(resource.getrusage(resource.RUSAGE_SELF))
    print('RAM Used (GB):', psutil.virtual_memory()[3]/1000000000)
