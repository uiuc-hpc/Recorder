#!/usr/bin/env python
# encoding: utf-8
import sys, struct
import numpy as np
import time

func_list = [
    # POSIX I/O - 66 functions
    "creat",        "creat64",      "open",         "open64",   "close",
    "write",        "read",         "lseek",        "lseek64",  "pread",
    "pread64",      "pwrite",       "pwrite64",     "readv",    "writev",
    "mmap",         "mmap64",       "fopen",        "fopen64",  "fclose",
    "fwrite",       "fread",        "ftell",        "fseek",    "fsync",
    "fdatasync",    "__xstat",      "__xstat64",    "__lxstat", "__lxstat64",
    "__fxstat",     "__fxstat64",   "getcwd",       "mkdir",    "rmdir",
    "chdir",        "link",         "linkat",       "unlink",   "symlink",
    "symlinkat",    "readlink",     "readlinkat",   "rename",   "chmod",
    "chown",        "lchown",       "utime",        "opendir",  "readdir",
    "closedir",     "rewinddir",    "mknod",        "mknodat",  "fcntl",
    "dup",          "dup2",         "pipe",         "mkfifo",   "umask",
    "fdopen",       "fileno",       "access",       "faccessat","tmpfile",
    "remove",

    # MPI I/O  - 71 functions
    "PMPI_File_close",              "PMPI_File_set_size",       "PMPI_File_iread_at",
    "PMPI_File_iread",              "PMPI_File_iread_shared",   "PMPI_File_iwrite_at",
    "PMPI_File_iwrite",             "PMPI_File_iwrite_shared",  "PMPI_File_open",
    "PMPI_File_read_all_begin",     "PMPI_File_read_all",       "PMPI_File_read_at_all",
    "PMPI_File_read_at_all_begin",  "PMPI_File_read_at",        "PMPI_File_read",
    "PMPI_File_read_ordered_begin", "PMPI_File_read_ordered",   "PMPI_File_read_shared",
    "PMPI_File_set_view",           "PMPI_File_sync",           "PMPI_File_write_all_begin",
    "PMPI_File_write_all",          "PMPI_File_write_at_all_begin", "PMPI_File_write_at_all",
    "PMPI_File_write_at",           "PMPI_File_write",          "PMPI_File_write_ordered_begin",
    "PMPI_File_write_ordered",      "PMPI_File_write_shared",   "PMPI_Finalize",
    "PMPI_Finalized",               "PMPI_Init",                "PMPI_Init_thread",
    "PMPI_Wtime",                   "PMPI_Comm_rank",           "PMPI_Comm_size",
    "PMPI_Get_processor_name",      "PMPI_Get_processor_name",  "PMPI_Comm_set_errhandler",
    "PMPI_Barrier",                 "PMPI_Bcast",               "PMPI_Gather",
    "PMPI_Gatherv",                 "PMPI_Scatter",             "PMPI_Scatterv",
    "PMPI_Allgather",               "PMPI_Allgatherv",          "PMPI_Alltoall",
    "PMPI_Reduce",                  "PMPI_Allreduce",           "PMPI_Reduce_scatter",
    "PMPI_Scan",                    "PMPI_Type_commit",         "PMPI_Type_contiguous",
    "PMPI_Type_extent",             "PMPI_Type_free",           "PMPI_Type_hindexed",
    "PMPI_Op_create",               "PMPI_Op_free",             "PMPI_Type_get_envelope",
    "PMPI_Type_size",
    # Added 2019/01/07
    "PMPI_Cart_rank",               "PMPI_Cart_create",         "PMPI_Cart_get",
    "PMPI_Cart_shift",              "PMPI_Wait",                "PMPI_Send",
    "PMPI_Recv",                    "PMPI_Sendrecv",            "PMPI_Isend",
    "PMPI_Irecv",

    # HDF5 I/O - 68 functions
    "H5Fcreate",            "H5Fopen",              "H5Fclose",     # File interface
    "H5Gclose",             "H5Gcreate1",           "H5Gcreate2",   # Group interface
    "H5Gget_objinfo",       "H5Giterate",           "H5Gopen1",
    "H5Gopen2",             "H5Dclose",             "H5Dcreate1",
    "H5Dcreate2",           "H5Dget_create_plist",  "H5Dget_space", # Dataset interface
    "H5Dget_type",          "H5Dopen1",             "H5Dopen2",
    "H5Dread",              "H5Dwrite",             "H5Sclose",
    "H5Screate",            "H5Screate_simple",     "H5Sget_select_npoints",    # Dataspace interface
    "H5Sget_simple_extent_dims", "H5Sget_simple_extent_npoints", "H5Sselect_elements",
    "H5Sselect_hyperslab",  "H5Sselect_none",       "H5Tclose",     # Datatype interface
    "H5Tcopy",              "H5Tget_class",         "H5Tget_size",
    "H5Tset_size",          "H5Tcreate",            "H5Tinsert",
    "H5Aclose",             "H5Acreate1",           "H5Acreate2",   # Attribute interface
    "H5Aget_name",          "H5Aget_num_attrs",     "H5Aget_space",
    "H5Aget_type",          "H5Aopen",              "H5Aopen_idx",
    "H5Aopen_name",         "H5Aread",              "H5Awrite",
    "H5Pclose",             "H5Pcreate",            "H5Pget_chunk", # Property List interface
    "H5Pget_mdc_config",    "H5Pset_alignment",     "H5Pset_chunk",
    "H5Pset_dxpl_mpio",     "H5Pset_fapl_core",     "H5Pset_fapl_mpio",
    "H5Pset_fapl_mpiposix", "H5Pset_istore_k",      "H5Pset_mdc_config",
    "H5Pset_meta_block_size","H5Lexists",           "H5Lget_val",   # Link interface
    "H5Literate",           "H5Oclose",             "H5Oget_info",  # Object interface
    "H5Oget_info_by_name",  "H5Oopen"
]

'''
Global Metadata Structure
    Time Resolution:        double, 8 bytes
    Number of MPI Ranks:    Int, 4 bytes
    Compression Mode:       Int, 4 bytes
    Recorder Window Size:   Int ,4 bytes
'''
class GlobalMetadata:
    def __init__(self, path):
        self.timeResolution = 0
        self.numRanks = 0
        self.compMode = 0
        self.windowSize = 0

        self.read(path)
        self.output()

    def read(self, path):
        with open(path, 'rb') as f:
            buf = f.read(8+4+4+4)
            self.timeResolution, self.numRanks, \
            self.compMode, self.windowSize = struct.unpack("diii", buf)

    def output(self):
        print("Time Resolution:", self.timeResolution)
        print("Ranks:", self.numRanks)
        print("Compression Mode:", self.compMode)
        print("Window Sizes:", self.windowSize)

'''
double start_timestamp;
double end_timestamp;
int num_files;                  // number of files accessed by the rank
int total_records;              // total number of records we have written
char **filemap;                 // mapping of filenames and integer ids. only set when read the local def file
size_t *file_sizes;             // size of each file accessed. only set when read back the local def file
int function_count[256];        // counting the functions at runtime
'''
class LocalMetadata:
    def __init__(self, path):
        self.tstart = 0
        self.tend = 0
        self.numFiles = 0
        self.totalRecords = 0
        self.functionCounter = []
        self.fileMap = []

        self.read(path)

    def read(self, path):
        with open(path, 'rb') as f:
            self.tstart, self.tend = struct.unpack("dd", f.read(8+8))
            self.numFiles, self.totalRecords = struct.unpack("ii", f.read(4+4))

            # ignore the two pointers
            f.read(8+8)

            self.functionCounter = struct.unpack("i"*256, f.read(256*4))

            for i in range(self.numFiles):
                fileId = struct.unpack("i", f.read(4))[0]
                fileSize = struct.unpack("l", f.read(8))[0]
                filenameLen = struct.unpack("i", f.read(4))[0]
                filename = f.read(filenameLen)
                self.fileMap.append([fileId, fileSize, filename])

    def output(self):
        print("Basic Information:")
        print("tstart:", self.tstart)
        print("tend:", self.tend)
        print("files:", self.numFiles)
        print("records:", self.totalRecords)

        print("\nFunction Counter:")
        for idx, count in enumerate(self.functionCounter):
            if count > 0: print(func_list[idx], count)
        print("\nFile Map:")
        for fileInfo in self.fileMap:
            print(fileInfo)

class RecorderReader:
    def __init__(self, path):
        self.globalMetadata = GlobalMetadata(path+"/recorder.mt")
        self.localMetadata = []
        self.records = []

        self.totalRecords = 0
        self.compressedRecords = 0

        self.totalArgs = 0;
        self.diffArgs = 0


        readTime, decodeTime, decompressTime = 0.0, 0.0, 0.0
        for rank in range(self.globalMetadata.numRanks):
            self.localMetadata.append( LocalMetadata(path+"/"+str(rank)+".mt") )

            '''
            t = time.time()
            lines = self.readTraceFile( path+"/"+str(rank)+".itf" )
            readTime += (time.time() - t)

            t = time.time()
            records = self.decode(lines)
            decodeTime += (time.time() - t)

            t = time.time()
            records = self.decompress(records)
            decompressTime += (time.time() - t)

            self.records.append( records )
            '''

            #self.compressedFunctions(path+"/"+str(rank)+".itf")
        #print(self.totalArgs, self.totalArgs-self.diffArgs, (self.totalArgs-self.diffArgs)*1.0/self.totalArgs)
        #print(self.totalRecords, self.compressedRecords)
        #print("Read Time: %s\nDecode Time: %s\nDecompress Time: %s\n" %(readTime, decodeTime, decompressTime))

    def decompress(self, records):
        for idx, record in enumerate(records):
            if record[0] != 0:
                status, ref_id = record[0], record[3]
                records[idx][3] = records[idx-1-ref_id][3]
                binStr = bin(status & 0b11111111)   # use mask to get the two's complement as in C code
                binStr = binStr[3:][::-1]           # ignore the leading "0b1" and reverse the string

                refArgs = list(records[idx-1-ref_id][4])    # copy the list
                ii = 0
                for i, c in enumerate(binStr):
                    if c == '1':
                        if ii >= len(record[4]):
                            print(record, ii)
                        refArgs[i] = record[4][ii]
                        ii += 1
                records[idx][4] = refArgs
        return records

    '''
    The lines read in from one log file
    '''
    def decode(self, lines):
        records = []
        for line in lines:
            status = struct.unpack('b', line[0])[0]
            tstart = struct.unpack('i', line[1:5])[0]
            tend = struct.unpack('i', line[5:9])[0]
            funcId = struct.unpack('B', line[9])[0]
            args = line[11:].split(' ')
            funcname = func_list[funcId]

            #if ("open" in funcname or "read" in funcname or "write" in funcname) \
            #    and ("MPI" not in funcname) and ("H5" not in funcname):
            records.append([status, tstart, tend, funcId, args])
            if status != 0:
                self.compressedRecords += 1

        self.totalRecords += len(lines)
        return records


    '''
    One line per record:
        status:         singed char, 1 byte
        delta_tstart:   int, 4 bytes
        delta_tend:     int, 4 bytes
        funcId/refId:   signed char, 1 byte
        args:           string seperated by space
    Output is a list of records for one rank, where each  record has the format
        [status, tstart, tend, funcId/refId, [args]]
    '''
    def readTraceFile(self, path):
        print(path)
        lines = []
        with open(path, 'rb') as f:
            content = f.read()
            start_pos = 0
            end_pos = 0
            while True:
                end_pos = content.find("\n", start_pos+10)
                if end_pos == -1:
                    break
                line = content[start_pos: end_pos]
                lines.append(line)
                start_pos = end_pos+1
        return lines


    def compressedFunctions(self, path):
        records = []
        lines = []
        with open(path, 'rb') as f:
            content = f.read()
            start_pos = 0
            end_pos = 0
            while True:
                end_pos = content.find("\n", start_pos+10)
                if end_pos == -1:
                    break
                line = content[start_pos: end_pos]
                lines.append(line)
                start_pos = end_pos+1


        self.totalRecords += len(lines)
        for i in range(len(lines)):
            line = lines[i]
            status = struct.unpack('b', line[0])[0]
            tstart = struct.unpack('i', line[1:5])[0]
            tend = struct.unpack('i', line[5:9])[0]
            funcId = struct.unpack('B', line[9])[0]
            args = line[10:].strip().split(' ')
            funcname = func_list[funcId]

            numArgs = len(args)
            if status != 0:
                numArgs = records[i-1-funcId]
                self.totalArgs += numArgs
                self.diffArgs += len(args)
                self.compressedRecords += 1
            records.append(numArgs)
        print(path, len(lines))
