#!/usr/bin/env python
# encoding: utf-8
import sys
from creader_wrapper import RecorderReader

def handle_data_operations(record, fileMap, offsetBook, func_list, endOfFile):

    def update_end_of_file(rank, fd, filename, endOfFile, offsetBook):
        if filename in endOfFile and fd in offsetBook:
            endOfFile[filename][rank] = max(endOfFile[filename][rank], offsetBook[fd][rank])
        elif filename not in endOfFile:
            endOfFile[filename] = [0] * len(offsetBook[0])
            endOfFile[filename][rank] = offsetBook[fd][rank]
        else:
            print("Not possible: ", rank, fd, filename)
            endOfFile[filename][rank] = 0

    func = func_list[record.func_id]
    rank, args = record.rank, record.args_to_strs()

    filename, offset, count = "", -1, -1

    # Ignore the functions that may confuse later conditions test
    if "readlink" in func or "dir" in func:
        return filename, offset, count

    if "writev" in func or "readv" in func:
        fd, count = int(args[0]), int(args[1])
        if(fd not in fileMap): return "", -1, -1
        filename = fileMap[fd]

        offset = offsetBook[fd][rank]
        offsetBook[fd][rank] += count
        update_end_of_file(rank, fd, filename, endOfFile, offsetBook)
    elif "fwrite" in func or "fread" in func:
        fd, size, count = int(args[3]), int(args[1]), int(args[2])
        if(fd not in fileMap): return "", -1, -1
        filename = fileMap[fd]

        offset, count = offsetBook[fd][rank], size*count
        offsetBook[fd][rank] += count
        update_end_of_file(rank, fd, filename, endOfFile, offsetBook)
    elif "pwrite" in func or "pread" in func:
        fd, count, offset = int(args[0]), int(args[2]), int(args[3])
        if(fd not in fileMap): return "", -1, -1
        filename = fileMap[fd]

        update_end_of_file(rank, fd, filename, endOfFile, offsetBook)
    elif "write" in func or "read" in func:
        fd, count = int(args[0]), int(args[2])
        if(fd not in fileMap): return "", -1, -1
        filename = fileMap[fd]

        offset = offsetBook[fd][rank]
        offsetBook[fd][rank] += count
        update_end_of_file(rank, fd, filename, endOfFile, offsetBook)
    elif "fprintf" in func:
        fd, count = int(args[0]), int(args[1])
        if(fd not in fileMap): return "", -1, -1
        filename = fileMap[fd]

        offset = offsetBook[fd][rank]
        offsetBook[fd][rank] += count
        update_end_of_file(rank, fd, filename, endOfFile, offsetBook)

    return filename, offset, count


def handle_metadata_operations(record, fileMap, offsetBook, func_list, closeBook, segmentBook, endOfFile):

    def get_latest_offset(filename, rank, closeBook, endOfFile):
        # Every filename should be in endOfFile becuase we initialized it at the begining
        if filename not in closeBook and filename not in endOfFile:
            print("Encounter an unknown filename "+filename+". Should abort!")
            return 0
        if filename in closeBook:
            return max(endOfFile[filename][rank], closeBook[filename])
        else:
            return endOfFile[filename][rank]

    def create_new_segment(filename, rank, segmentBook):
        newSegmentID = 0
        if filename in segmentBook and len(segmentBook[filename]) > 0:
            newSegmentID = 1+segmentBook[filename][-1][1]
        if filename not in segmentBook:
            segmentBook[filename] = []
        segmentBook[filename].append([rank, newSegmentID, False])

    rank, func = record.rank, func_list[record.func_id]
    args = record.args_to_strs()

    # Ignore directory related operations
    if "dir" in func:
        return

    if "fopen" in func or "fdopen" in func:
        fd = record.res
        if "fdopen" in func:
            oldFd = int(args[0])
            if oldFd not in fileMap:        # openning some file that we do not know?
                return
            else:
                filename = fileMap[oldFd]
        else:
            filename = args[0]
        fileMap[fd] = filename
        offsetBook[fd][rank] = 0
        openMode = args[1]
        if 'a' in openMode:
            offsetBook[fd][rank] = get_latest_offset(filename, rank, closeBook, endOfFile)
        create_new_segment(filename, rank, segmentBook)
    elif "open" in func:
        fd = record.res
        filename = args[0]
        fileMap[fd] = filename
        offsetBook[fd][rank] = 0
        openMode = int( args[1] )
        if openMode == 2:  # TODO need  a better way to test for O_APPEND
            offsetBook[fd][rank] = get_latest_offset(filename, rank, closeBook, endOfFile)
        create_new_segment(filename, rank, segmentBook)
    elif "seek" in func:
        fd, offset, whence = int(args[0]), int(args[1]), int(args[2])
        if fd not in fileMap: return
        filename = fileMap[fd]

        if whence == 0:     # SEEK_SET
            offsetBook[fd][rank] = offset
        elif whence == 1:   # SEEK_CUR
            offsetBook[fd][rank] += offset
        elif whence == 2:   # SEEK_END
            offsetBook[fd][rank] = get_latest_offset(filename, rank, closeBook, endOfFile)

    elif "close" in func or "sync" in func:
        fd = int(args[0])
        if(fd not in fileMap): return
        filename = fileMap[fd]
        if "close" in func: del fileMap[fd]

        closeBook[filename] = endOfFile[filename][rank]

        # 1. Close all segments on the local process for this file
        for i in range(len(segmentBook[filename])):
            if segmentBook[filename][i][0] == rank:
                segmentBook[filename][i][2] = True
        # 2. And starts a new segment for all other processes have the same file opened
        # Skip this step for session semantics
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


def ignore_files(filename):
    if not filename or filename == "":
        return True
    ignore_prefixes = ["/sys/", "/dev", "/proc", "/p/lustre2/wang116/applications/ParaDis.v2.5.1.1/Copper/Copper_results/fluxdata/", "/etc/", "stdout", "stderr", "stdin"]
    for prefix in ignore_prefixes:
        if filename.startswith(prefix):
            return True
    if "pipe:" in filename:
        return True

    return False

def ignore_funcs(func):
    ignore = ["MPI", "H5", "writev"]
    for f in ignore:
        if f in func:
            return True
    return False


def build_offset_intervals(reader):
    func_list = reader.funcs
    ranks = reader.GM.total_ranks

    closeBook = {}  # Keep track the most recent close function and its file size so a later append operation knows the most recent file size
    segmentBook = {}    # segmentBook[filename] maintains all segments for filename, it is a list of list (rank, segment-id, closed)
    offsetBook = {}
    endOfFile = {}  # endOfFile[filename][rank] keep tracks the end of file, only the local rank can see it. When close/fsync, the value is stored in closeBook so other rank can see it.
    intervals = {}

    # merge the list(reader.records) of list(each rank's records) into one flat list
    # then sort the whole list by tstart
    records = []
    for rank in range(ranks):
        for i in range(reader.LMs[rank].total_records):
            record = reader.records[rank][i]
            record.rank = rank
            if not ignore_funcs(func_list[record.func_id]):
                records.append( record )

    records = sorted(records, key=lambda x: x.tstart)

    # Initialize offset book, each rank maintains its own offset book
    fdSet = set([0, 1, 2])  # stdin, stderr, stdout
    for record in records:
        fdSet.add(record.res)
    for fd in fdSet:
        offsetBook[fd] = [0] * ranks

    segmentBook["stdin"] = []
    segmentBook["stderr"] = []
    segmentBook["stdout"] = []
    endOfFile["stdin"] = [0] * ranks
    endOfFile["stderr"] = [0] * ranks
    endOfFile["stdout"] = [0] * ranks

    for rank in range(ranks):
        LM = reader.LMs[rank]
        for i in range(LM.num_files):
            filename = LM.filenames[i].replace(" ", "_")
            segmentBook[filename] = []
            endOfFile[filename] = [0] * ranks

    fileMaps = []
    for i in range(ranks):
        fileMaps.append({0: "stdin", 1: "stdout", 2: "stderr"})


    for record in records:

        rank = record.rank
        fileMap = fileMaps[rank]

        func = func_list[record.func_id]

        handle_metadata_operations(record, fileMap, offsetBook, func_list, closeBook, segmentBook, endOfFile)
        filename, offset, count = handle_data_operations(record, fileMap, offsetBook, func_list, endOfFile)

        if not ignore_files(filename):
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
            intervals[filename].append( [rank, record.tstart, record.tend, offset, count, isRead, segments] )

    return intervals

if __name__ == "__main__":
    reader = RecorderReader(sys.argv[1])
    build_offset_intervals(reader)
