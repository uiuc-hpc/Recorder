#!/usr/bin/env python
# encoding: utf-8
import sys
from reader import RecorderReader

version = 2.1

def handle_data_operations(record, fileMap, offsetBook, func_list, endOfFile):
    func = func_list[record.funcId]
    rank, args = record.rank, record.args

    filename, offset, count = "", -1, -1

    # Ignore the functions that may confuse later conditions test
    if "readlink" in func or "dir" in func:
        return filename, offset, count

    if "writev" in func or "readv" in func:
        fileId, count = int(args[0]), int(args[1])
        if version >= 2.1:
            if(fileId not in fileMap): return "", -1, -1
            filename = fileMap[fileId]
            fdOrFilename = fileId
        else:
            filename = fileMap[fileId][2]
            fdOrFilename = filename

        offset = offsetBook[fdOrFilename][rank]
        offsetBook[fdOrFilename][rank] += count
        endOfFile[filename][rank] = max(endOfFile[filename][rank], offsetBook[fdOrFilename][rank])
    elif "fwrite" in func or "fread" in func:
        fileId, size, count = int(args[3]), int(args[1]), int(args[2])
        if version >= 2.1:
            if(fileId not in fileMap): return "", -1, -1
            filename = fileMap[fileId]
            fdOrFilename = fileId
        else:
            filename = fileMap[fileId][2]
            fdOrFilename = filename

        offset, count = offsetBook[fdOrFilename][rank], size*count
        offsetBook[fdOrFilename][rank] += count
        endOfFile[filename][rank] = max(endOfFile[filename][rank], offsetBook[fdOrFilename][rank])
    elif "pwrite" in func or "pread" in func:
        fileId, count, offset = int(args[0]), int(args[2]), int(args[3])
        if version >= 2.1:
            if(fileId not in fileMap): return "", -1, -1
            filename = fileMap[fileId]
        else:
            filename = fileMap[fileId][2]
        endOfFile[filename][rank] = max(endOfFile[filename][rank], offset+count)
    elif "write" in func or "read" in func:
        fileId, count = int(args[0]), int(args[2])
        if version >= 2.1:
            if(fileId not in fileMap): return "", -1, -1
            filename = fileMap[fileId]
            fdOrFilename = fileId
        else:
            filename = fileMap[fileId][2]
            fdOrFilename = filename

        offset = offsetBook[fdOrFilename][rank]
        offsetBook[fdOrFilename][rank] += count
        endOfFile[filename][rank] = max(endOfFile[filename][rank], offsetBook[fdOrFilename][rank])
    elif "fprintf" in func:
        fileId, count = int(args[0]), int(args[1])
        if version >= 2.1:
            if(fileId not in fileMap): return "", -1, -1
            filename = fileMap[fileId]
            fdOrFilename = fileId
        else:
            filename = fileMap[fileId][2]
            fdOrFilename = filename

        offset = offsetBook[fdOrFilename][rank]
        offsetBook[fdOrFilename][rank] += count
        endOfFile[filename][rank] = max(endOfFile[filename][rank], offsetBook[fdOrFilename][rank])

    return filename, offset, count


def handle_metadata_operations(record, fileMap, offsetBook, func_list, closeBook, segmentBook, endOfFile):
    rank, func = record.rank, func_list[record.funcId]
    # Ignore directory related operations
    if "dir" in func:
        return

    if "fopen" in func or "fdopen" in func:
        # version 2.1 and new, we keep the returned fd, and the argument is just the full file path
        if version >= 2.1:
            fd = record.res
            filename = record.args[0]
            fileMap[fd] = filename
            offsetBook[fd][rank] = 0
            # Need to find out the correct file size from the closeBook
            openMode = record.args[1]
            if 'a' in openMode:
                offsetBook[fd][rank] = max(endOfFile[filename][rank], closeBook[filename]) if filename in closeBook else endOfFile[filename][rank]
        else:
            fileId = int(record.args[0])
            filename = fileMap[fileId][2]
            offsetBook[filename][rank] = 0
            # Need to find out the correct file size from the closeBook
            openMode = record.args[1]
            if 'a' in openMode:
                offsetBook[filename][rank] = max(endOfFile[filename][rank], closeBook[filename]) if filename in closeBook else endOfFile[filename][rank]

        # create a new segment
        newSegmentID = 1+segmentBook[filename][-1][1] if len(segmentBook[filename]) > 0 else 0
        segmentBook[filename].append([rank, newSegmentID, False])
    elif "open" in func:
        if version >= 2.1:
            fd = record.res
            filename = record.args[0]
            fileMap[fd] = filename
            offsetBook[fd][rank] = 0
        else:
            fileId = int(record.args[0])
            filename = fileMap[fileId][2]
            offsetBook[filename][rank] = 0
        # create a new segment
        newSegmentID = 1+segmentBook[filename][-1][1] if len(segmentBook[filename]) > 0 else 0
        segmentBook[filename].append([rank, newSegmentID, False])
        # TODO consider append flags
    elif "seek" in func:
        fileId, offset, whence = int(record.args[0]), int(record.args[1]), int(record.args[2])
        if version >= 2.1:
            if fileId not in fileMap: return
            filename = fileMap[fileId]
            fdOrFilename = fileId
        else:
            filename = fileMap[fileId][2]
            fdOrFilename = filename

        if whence == 0:     # SEEK_SET
            offsetBook[fdOrFilename][rank] = offset
        elif whence == 1:   # SEEK_CUR
            offsetBook[fdOrFilename][rank] += offset
        elif whence == 2:   # SEEK_END
            offsetBook[fdOrFilename][rank] = max(endOfFile[filename][rank], closeBook[filename]) if filename in closeBook else endOfFile[filename][rank]

    elif "close" in func or "sync" in func:
        fileId = int(record.args[0])
        if version >= 2.1:
            if(fileId not in fileMap): return
            filename = fileMap[fileId]
            fdOrFilename = fileId
        else:
            filename = fileMap[fileId][2]
            fdOrFilename = filename

        closeBook[filename] = endOfFile[filename][rank]

        # 1. Close all segments on the local process for this file
        for i in range(len(segmentBook[filename])):
            if segmentBook[filename][i][0] == rank:
                segmentBook[filename][i][2] = True
        # 2. And starts a new segment for all other processes have the same file opened
        # Skip this step for session semantics
        '''
        visitedRanks = set()
        tmpSegments = []
        # [::-1] check the most recent unclosed segment to get the largest segmentId
        for segment in segmentBook[filename][::-1]:
            if segment[0] in visitedRanks:
                continue
            if segment[0] != rank and not segment[2]:
                tmpSegments.append([segment[0], 1+segment[1], False])
                visitedRanks.add(segment[0])
        segmentBook[filename] = segmentBook[filename] + tmpSegments
        '''



def ignore_files(filename):
    ignore_prefixes = ["/sys/", "/dev", "/proc", "/p/lustre2/wang116/applications/ParaDis.v2.5.1.1/Copper/Copper_results/fluxdata/"]
    for prefix in ignore_prefixes:
        if filename.startswith(prefix):
            return True
    if "pipe:" in filename:
        return True

    return False


def build_offset_intervals(reader):
    version = reader.globalMetadata.version
    func_list = reader.globalMetadata.funcs
    timeRes = reader.globalMetadata.timeResolution
    ranks = reader.globalMetadata.numRanks

    closeBook = {}  # Keep track the most recent close function and its file size so a later append operation knows the most recent file size
    segmentBook = {}    # segmentBook[filename] maintains all segments for filename, it is a list of list (rank, segment-id, closed)
    offsetBook = {}
    endOfFile = {}  # endOfFile[filename][rank] keep tracks the end of file, only the local rank can see it. When close/fsync, the value is stored in closeBook so other rank can see it.
    intervals = {}

    # merge the list(reader.records) of list(each rank's records) into one flat list
    # then sort the whole list by tstart
    records = []
    for rank in range(ranks):
        records = records + reader.records[rank]
    records = sorted(records, key=lambda x: x.tstart)


    # Initialize offset book, each rank maintains its own offset book
    for localMetadata in reader.localMetadata:
        if version >= 2.1:
            fdSet = set([0, 1, 2])
            for record in records:
                fdSet.add(record.res)
            for fd in fdSet:
                offsetBook[fd] = [0] * ranks

            for fileInfo in localMetadata.fileMap:
                filename = fileInfo[2]
                segmentBook[filename] = []
                endOfFile[filename] = [0] * ranks
            segmentBook["stdin"] = []
            segmentBook["stderr"] = []
            segmentBook["stdout"] = []
            endOfFile["stdin"] = [0] * ranks
            endOfFile["stderr"] = [0] * ranks
            endOfFile["stdout"] = [0] * ranks


        else:
            for fileInfo in localMetadata.fileMap:
                filename = fileInfo[2]
                offsetBook[filename] = [0] * ranks
                endOfFile[filename] = [0] * ranks
                segmentBook[filename] = []

    fileMaps = []
    for i in range(ranks): fileMaps.append({0: "stdin", 1: "stdout", 2: "stderr"})


    for record in records:

        rank = record.rank

        if reader.globalMetadata.version >= 2.1:
            fileMap = fileMaps[rank]
        else:
            fileMap = reader.localMetadata[rank].fileMap

        func = func_list[record.funcId]
        if "MPI" in func or "H5" in func: continue

        handle_metadata_operations(record, fileMap, offsetBook, func_list, closeBook, segmentBook, endOfFile)
        filename, offset, count = handle_data_operations(record, fileMap, offsetBook, func_list, endOfFile)

        if(filename != "" and not ignore_files(filename)):
            tstart = timeRes * int(record.tstart)
            tend = timeRes * int(record.tend)
            isRead = "read" in func
            if filename not in intervals:
                intervals[filename] = []

            # segments[0] is the local segment, the others are remote segments
            segments = []
            # 1. Add local segment
            # Find the most recent unclosed local segment
            for segment in segmentBook[filename][::-1]:
                if segment[0] == rank and not segment[2]:
                    segments.append(segment[1])

            # 2. Add all remote segments
            for segment in segmentBook[filename]:
                if segment[0] != rank and not segment[2]:
                    segments.append(segment[1])
            intervals[filename].append( [rank, tstart, tend, offset, count, isRead, segments] )

    return intervals

if __name__ == "__main__":
    reader = RecorderReader(sys.argv[1])
    build_offset_intervals(reader)
