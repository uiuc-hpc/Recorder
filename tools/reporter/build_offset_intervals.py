#!/usr/bin/env python
# encoding: utf-8
import sys
from reader import RecorderReader, func_list


def handle_data_operations(record, fileMap, offsetBook):
    func = func_list[record[3]]
    rank, args = record[-1], record[4]

    filename, offset, count = "", -1, -1

    if "writev" in func or "readv" in func:
        fileId, count = int(args[0]), int(args[1])
        filename = fileMap[fileId][2]
        offset = offsetBook[filename][rank]
        offsetBook[filename][rank] += count
    elif "fwrite" in func or "fread" in func:
        fileId, size, count = int(args[3]), int(args[1]), int(args[2])
        filename = fileMap[fileId][2]
        offset, count = offsetBook[filename][rank], size*count
        offsetBook[filename][rank] += count
    elif "pwrite" in func or "pread" in func:
        fileId, count, offset = int(args[0]), int(args[2]), int(args[3])
        filename = fileMap[fileId][2]
    elif "write" in func or "read" in func:
        fileId, count = int(args[0]), int(args[2])
        filename = fileMap[fileId][2]
        offset = offsetBook[filename][rank]
        offsetBook[filename][rank] += count

    return filename, offset, count


def handle_metadata_operations(record, fileMap, offsetBook):
    rank, func = record[-1], func_list[record[3]]
    # TODO consider append flags
    if "fopen" in func or "fdopen" in func:
        fileId = int(record[4][0])
        filename = fileMap[fileId][2]
        offsetBook[filename][rank] = 0
    elif "open" in func:
        fileId = int(record[4][0])
        filename = fileMap[fileId][2]
        offsetBook[filename][rank] = 0
    elif "seek" in func:
        fileId, offset, whence = int(record[4][0]), int(record[4][1]), int(record[4][2])
        filename = fileMap[fileId][2]
        if whence == 0:     # SEEK_STE
            offsetBook[filename][rank] = offset
        elif whence == 1:   # SEEK_CUR
            offsetBook[filename][rank] += offset

def ignore_files(filename):
    ignore_prefixes = ["/dev", "/proc"]
    for prefix in ignore_prefixes:
        if filename.startswith(prefix):
            return True
    if "pipe:" in filename:
        return True

    return False


def build_offset_intervals(reader):
    timeRes = reader.globalMetadata.timeResolution
    ranks = reader.globalMetadata.numRanks

    offsetBook = {}
    intervals = {}

    # Initialize offset book, each rank maintains its own offset book
    for localMetadata in reader.localMetadata:
        for fileInfo in localMetadata.fileMap:
            filename = fileInfo[2]
            offsetBook[filename] = [0] * ranks


    # merge the list(reader.records) of list(each rank's records) into one flat list
    # then sort the whole list by tstart
    records = []
    for rank in range(ranks):
        for record in reader.records[rank]:
            records.append(record+[rank])             # insert rank at the end
    records = sorted(records, key=lambda x: x[1])

    for record in records:
        rank = record[-1]
        fileMap = reader.localMetadata[rank].fileMap

        func = func_list[record[3]]
        if "MPI" in func or "H5" in func: continue

        handle_metadata_operations(record, fileMap, offsetBook)
        filename, offset, count = handle_data_operations(record, fileMap, offsetBook)
        if(filename != "" and not ignore_files(filename)):
            tstart = timeRes * int(record[1])
            tend = timeRes * int(record[2])
            isRead = "read" in func
            if filename not in intervals:
                intervals[filename] = []
            intervals[filename].append( [rank, tstart, tend, offset, count, isRead] )

    return intervals

if __name__ == "__main__":
    reader = RecorderReader(sys.argv[1])
    build_offset_intervals(reader)
