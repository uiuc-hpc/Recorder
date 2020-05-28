#!/usr/bin/env python
# encoding: utf-8
import sys, struct
import numpy as np
import time

'''
Global Metadata Structure - same as in include/recorder-log-format.h
    Time Resolution:        double, 8 bytes
    Number of MPI Ranks:    Int, 4 bytes
    Compression Mode:       Int, 4 bytes
    Recorder Window Size:   Int ,4 bytes

    Then followed by one function name per line
    They are the one intercepted by the current version of Recorder
'''
class GlobalMetadata:
    def __init__(self, path):
        self.timeResolution = 0
        self.numRanks = 0
        self.compMode = 0
        self.windowSize = 0
        self.funcs = []
        self.version = 2.1

        self.read(path)
        self.output()

    def read(self, path):
        with open(path, 'rb') as f:
            buf = f.read(8+4+4+4)
            self.timeResolution, self.numRanks, \
            self.compMode, self.windowSize = struct.unpack("diii", buf)

            f.seek(24, 0)
            self.funcs = f.read().splitlines()
            self.funcs = [func.replace("PMPI", "MPI") for func in self.funcs]


    def output(self):
        print("Time Resolution:", self.timeResolution)
        print("Ranks:", self.numRanks)
        print("Compression Mode:", self.compMode)
        print("Window Sizes:", self.windowSize)

'''
Local Metadata Structure - same as in include/recorder-log-format.h

    Start timestamp:            double, 8 bytes
    ed timestamp:               double, 8 bytes
    Number of files accessed:   int,    4 bytes
    Total number of records:    int,    4 bytes
    Filemap:                    char**, 8 bytes pointer, ignore it
    File sizes array:           int*,   8 bytes pointer, size of each file accessed, ignore it
    Function counter:           int*256,4 * 256 bytes

    Then one line per file accessed, has the following form:
    file id(4), file size(8), filename length(4), filename(variable length)
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

            self.fileMap = [None] * self.numFiles
            for i in range(self.numFiles):
                fileId = struct.unpack("i", f.read(4))[0]
                fileSize = struct.unpack("l", f.read(8))[0]
                filenameLen = struct.unpack("i", f.read(4))[0]
                filename = f.read(filenameLen)
                self.fileMap[fileId] = [fileId, fileSize, filename]

    def output(self):
        print("Basic Information:")
        print("tstart:", self.tstart)
        print("tend:", self.tend)
        print("files:", self.numFiles)
        print("records:", self.totalRecords)

        #print("\nFunction Counter:")
        #for idx, count in enumerate(self.functionCounter):
        #    if count > 0: print(idx, count)
        print("\nFile Map:")
        for fileInfo in self.fileMap:
            print(fileInfo)

class Record:
    def __init__(self, rank, status, tstart, tend, funcId, args, res=None):
        self.rank = rank
        self.status = status
        self.tstart = tstart
        self.tend = tend
        self.funcId = funcId
        self.args = args
        self.res = res


class RecorderReader:
    def __init__(self, path, readMetadataOnly=False):
        self.globalMetadata = GlobalMetadata(path+"/recorder.mt")
        self.localMetadata = []
        self.records = []

        for rank in range(self.globalMetadata.numRanks):
            self.localMetadata.append( LocalMetadata(path+"/"+str(rank)+".mt") )
            if(readMetadataOnly): continue
            lines = self.read( path+"/"+str(rank)+".itf" )
            records = self.decode(lines, rank)
            records = self.decompress(records)
            # sort records by tstart
            records = sorted(records, key=lambda x: x.tstart)  # sort by tstart
            self.records.append( records )

    def decompress(self, records):
        for idx, record in enumerate(records):
            if record.status != 0:                              # compressed
                status, ref_id = record.status, record.funcId
                records[idx].funcId = records[idx-1-ref_id].funcId
                binStr = bin(status & 0b11111111)               # use mask to get the two's complement as in C code
                binStr = binStr[3:][::-1]                       # ignore the leading "0b1" and reverse the string

                refArgs = list(records[idx-1-ref_id].args)      # copy the list
                ii = 0
                for i, c in enumerate(binStr):
                    if c == '1':
                        if ii >= len(record.args):
                            print("Error:", record, ii)
                        refArgs[i] = record.args[ii]
                        ii += 1
                records[idx].args = refArgs
        return records

    '''
    @lines: The lines read in from one log file
        status:         singed char, 1 byte
        delta_tstart:   int, 4 bytes
        delta_tend:     int, 4 bytes
        funcId/refId:   signed char, 1 byte
        args:           string seperated by space
    Output is a list of records for one rank, where each  record has the format
        [status, tstart, tend, funcId/refId, [args]]
    '''
    def decode(self, lines, rank):
        records = []
        for line in lines:
            status = struct.unpack('b', line[0])[0]
            tstart = struct.unpack('i', line[1:5])[0]
            tend = struct.unpack('i', line[5:9])[0]

            if(self.globalMetadata.version < 2.1):
                funcId = struct.unpack('B', line[9])[0]
                args = line[11:].split(' ')
                records.append(Record(rank, status, tstart, tend, funcId, args))
            else:
                # For Recorder 2.1 and new version
                res = struct.unpack('i', line[9:13])[0]
                funcId = struct.unpack('B', line[13])[0]
                args = line[15:].split(' ')
                records.append(Record(rank, status, tstart, tend, funcId, args, res))

        return records


    '''
    Read one rank's trace file and return one line for each record
    '''
    def read(self, path):
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
