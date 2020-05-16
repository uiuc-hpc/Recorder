#!/usr/bin/env python
# encoding: utf-8

from reader import RecorderReader
import sys, os

def fill_in_filename(record, fileMap, func_list):
    func = func_list[record.funcId]
    args = record.args

    if "H5" in func or "MPI" in func:
        return record

    if "mmap" in func:
        fileId = int(args[4])
        filename = fileMap[fileId][2]
        record.args[4] = filename
    elif "fwrite" in func or "fread" in func:
        fileId = int(args[3])
        filename = fileMap[fileId][2]
        record.args[3] = filename
    elif "dir" in func:       # ignore opendir, closedir, etc.
        pass
    elif "seek" in func or "open" in func or "close" in func \
        or "write" in func or "read" in func or "truncate" in func \
        or "unlink" in func or "fprintf" in func:
        fileId = int(args[0])
        filename = fileMap[fileId][2]
        record.args[0] = filename
    return record

# For Recorder 2.1 and newer
def fill_in_filename_2(record, fileMap, func_list):
    func = func_list[record.funcId]
    args = record.args

    if "H5" in func or "MPI" in func:
        return record

    if "open" in func or "creat" in func:
        if not "fdopen" in func:
            filename = args[0]
            fileMap[record.res] = filename
    # TODO: dup/dup2

    if "mmap" in func:
        fd = int(args[4])
        filename = fileMap[fd] if fd in fileMap else "unknow fd"
        record.args[4] = filename
    elif "fwrite" in func or "fread" in func:
        fd = int(args[3])
        filename = fileMap[fd] if fd in fileMap else "unknow fd"
        record.args[3] = filename
    elif "fxstat" in func:
        fd = int(args[1])
        filename = fileMap[fd] if fd in fileMap else "unknow fd"
        record.args[2] = filename
    elif "dir" in func:       # ignore opendir, closedir, etc.
        pass
    elif "seek" in func or "write" in func or "read" in func or "ftruncate" in func \
        or "close" in func or "fdopen" in func or "fileno" in func or "fprintf" in func:
        fd = int(args[0])
        filename = fileMap[fd] if fd in fileMap else "unknow fd"
        record.args[0] = filename

    return record


if __name__ == "__main__":
    logs_dir = sys.argv[1]+"/../logs_text"
    try:
        os.mkdir(logs_dir)
    except:
        pass

    reader = RecorderReader(sys.argv[1])
    version = reader.globalMetadata.version
    func_list = reader.globalMetadata.funcs
    for rank in range(reader.globalMetadata.numRanks):
        fileMap = reader.localMetadata[rank].fileMap
        if version >= 2.1:
            fileMap = {0: "stdin", 1: "stdout", 2:"stderr"}

        print(rank, len(reader.records[rank]))
        with open(logs_dir+"/"+str(rank)+".txt", 'w') as f:
            for record in reader.records[rank]:
                if version >= 2.1 :
                    record = fill_in_filename_2(record, fileMap, func_list)
                else:
                    record = fill_in_filename(record, fileMap, func_list)
                recordText = "%s %s %s %s\n" %(record.tstart, record.tend, func_list[record.funcId], record.args)
                f.write(recordText)
