#!/usr/bin/env python
# encoding: utf-8
import sys
from reader import RecorderReader


def handle_data_operations(record, fileMap, offsetBook, func_list):
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
    elif "fprintf" in func:
        fileId, count = int(args[0]), int(args[1])
        filename = fileMap[fileId][2]
        offset = offsetBook[filename][rank]
        offsetBook[filename][rank] += count

    return filename, offset, count


def handle_metadata_operations(record, fileMap, offsetBook, func_list, closeBook, sessionBook):
    rank, func = record[-1], func_list[record[3]]
    # TODO consider append flags
    if "fopen" in func or "fdopen" in func:
        fileId = int(record[4][0])
        filename = fileMap[fileId][2]
        offsetBook[filename][rank] = 0
        # Need to find out the correct file size from the closeBook
        openMode = record[4][1]
        if openMode == 'a':
            offsetBook[filename][rank] = closeBook[filename] if filename in closeBook else 0
        # create a new session
        newSessionID = 1+sessionBook[filename][-1][1] if len(sessionBook[filename]) > 0 else 0
        sessionBook[filename].append([rank, newSessionID, False])
    elif "open" in func:
        fileId = int(record[4][0])
        filename = fileMap[fileId][2]
        offsetBook[filename][rank] = 0
        # create a new session
        newSession = 1+sessionBook[filename][0] if len(sessionBook[filename]) > 0 else 0
        sessionBook[filename].append((rank, newSession, False))
    elif "seek" in func:
        fileId, offset, whence = int(record[4][0]), int(record[4][1]), int(record[4][2])
        filename = fileMap[fileId][2]
        if whence == 0:     # SEEK_STE
            offsetBook[filename][rank] = offset
        elif whence == 1:   # SEEK_CUR
            offsetBook[filename][rank] += offset
    elif "close" in func:
        fileId = int(record[4][0])
        filename = fileMap[fileId][2]
        closeBook[filename] = offsetBook[filename][rank]

        # close the most recent open session
        for i in range(len(sessionBook[filename]))[::-1]:
            if sessionBook[filename][i][0] == rank:
                sessionBook[filename][i][2] = True
                break

def ignore_files(filename):
    ignore_prefixes = ["/dev", "/proc", "/p/lustre2/wang116/applications/ParaDis.v2.5.1.1/Copper/Copper_results/fluxdata/"]
    for prefix in ignore_prefixes:
        if filename.startswith(prefix):
            return True
    if "pipe:" in filename:
        return True

    return False


def build_offset_intervals(reader):
    func_list = reader.globalMetadata.funcs
    timeRes = reader.globalMetadata.timeResolution
    ranks = reader.globalMetadata.numRanks

    closeBook = {}  # Keep track the most recent close function and its file size so a later append operation knows the most recent file size
    sessionBook = {}    # sessionBook[filename] maintains all sessions for filename, it is a list of list (rank, session-id, closed)
    offsetBook = {}
    intervals = {}

    # Initialize offset book, each rank maintains its own offset book
    for localMetadata in reader.localMetadata:
        for fileInfo in localMetadata.fileMap:
            filename = fileInfo[2]
            offsetBook[filename] = [0] * ranks
            sessionBook[filename] = []


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

        handle_metadata_operations(record, fileMap, offsetBook, func_list, closeBook, sessionBook)
        filename, offset, count = handle_data_operations(record, fileMap, offsetBook, func_list)
        if(filename != "" and not ignore_files(filename)):
            tstart = timeRes * int(record[1])
            tend = timeRes * int(record[2])
            isRead = "read" in func
            if filename not in intervals:
                intervals[filename] = []

            sessions = []
            startFlag = False
            for session in sessionBook[filename][::-1]:     # iterate from inner-most to outer-most
                if session[0] == rank and not session[2]:   # Find the inner most unclosed local session
                    startFlag = True
                if startFlag and not session[2]:            # keep adding unclosed remote sessions
                    sessions.append(session[1])

            intervals[filename].append( [rank, tstart, tend, offset, count, isRead, sessions] )

    return intervals

if __name__ == "__main__":
    reader = RecorderReader(sys.argv[1])
    build_offset_intervals(reader)
