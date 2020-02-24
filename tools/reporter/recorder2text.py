#!/usr/bin/env python
# encoding: utf-8

from reader import RecorderReader
import sys, os

def fill_in_filename(record, fileMap, func_list):
    func = func_list[record[3]]
    args = record[4]

    if "H5" in func or "MPI" in func:
        return record

    if "fwrite" in func or "fread" in func:
        fileId = int(args[3])
        filename = fileMap[fileId][2]
        record[4][3] = filename
    elif "dir" in func:       # ignore opendir, closedir, etc.
        pass
    elif "seek" in func or "open" in func or "close" in func \
         or "write" in func or "read" in func:
        fileId = int(args[0])
        filename = fileMap[fileId][2]
        record[4][0] = filename
    return record

if __name__ == "__main__":
    logs_dir = sys.argv[1]+"/../logs_text"
    try:
        os.mkdir(logs_dir)
    except:
        pass

    reader = RecorderReader(sys.argv[1])
    func_list = reader.globalMetadata.funcs
    for rank in range(reader.globalMetadata.numRanks):
        fileMap = reader.localMetadata[rank].fileMap

        print(rank, len(reader.records[rank]))
        with open(logs_dir+"/"+str(rank)+".txt", 'w') as f:
            for record in reader.records[rank]:
                record = fill_in_filename(record, fileMap, func_list)
                recordText = "%s %s %s %s\n" %(record[1], record[2], func_list[record[3]], record[4])
                f.write(recordText)
