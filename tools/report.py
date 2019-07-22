#!/usr/bin/env python
# encoding: utf-8
'''
The report contains the following part:
1. Number of files
2. A full file list with its size
3. List of function calls and how many times a function was called
'''
import glob, sys
import reader
from prettytable import PrettyTable
from html_writer import HTMLWriter
from vis import draw_bar_chart, draw_pie_chart
from vis import draw_offset_vs_rank

class TraceReader:
    def __init__(self, path):
        self.path = path                                # path to the log files
        self.procs = len(glob.glob(path+"/*.itf"))      # total number of processes

        self.data = None                                # trace log for each rank
        self.meta = None                                # meatadata for each rank

        self.posix_io = None                            # Only POSIX IO records stored in Pandas.DataFrame

    def get_metadata(self, rank):
        lines = None
        with open(self.path+"/"+str(rank)+".mt", 'r') as f:
            lines = f.readlines()
        lines = list( map(str.strip, lines) )       # remove the trailing newline
        return lines

    def get_data(self, rank):
        lines = None
        with open(self.path+"/"+str(rank)+".itf", 'r') as f:
            lines = f.readlines()
        from operator import itemgetter
        lines = sorted(lines, key=itemgetter(0))    # sort by timestemp
        lines = list( map(str.strip, lines) )       # remove the trailing newline
        return lines

    def get_posix_io(self):
        if self.data is None:
            from reader import read_traces
            self.data = read_traces(self.path)
            return self.data
        return self.data


def file_statistics(tr: TraceReader, html:HTMLWriter):
    fileTable = PrettyTable()
    header = ["", "Total"]
    for r in range(tr.procs): header.append("Rank "+str(r))
    fileTable.field_names = header
    row = ["Number of file accessed", 0]

    fileSizes = {}
    for rank in range(tr.procs):
        fileSet = set()
        lines = tr.get_metadata(rank)
        for record in lines:
            tmp = record.split(" ")
            filename = tmp[0].split('/')[-1]        # filename
            fileSizes[filename] = int(tmp[2])       # filesize
            fileSet.add(filename)

        row.append(len(fileSet))
    row[1] = len(fileSizes)
    fileTable.add_row(row)
    print(fileTable)
    html.fileTable = fileTable.get_html_string()

    # File sizes image
    x, y = [], []
    for name in fileSizes:
        x.append(name)
        y.append(fileSizes[name]/1024.0)
    html.fileSizeImage = "./figures/file_size.jpg"
    draw_bar_chart(x, y, title="File Size (KBytes)", save_to=html.fileSizeImage, horizontal=True, logScale=True, xlabel="File size (KBytes)")


def function_statistics(tr: TraceReader, html:HTMLWriter):
    functionCounter = {}
    for rank in range(tr.procs):
        lines = tr.get_data(rank)
        for record in lines:
            func = record.split(" ")[1]
            if func not in functionCounter:
                functionCounter[func] = [0] * tr.procs
            functionCounter[func][rank] += 1

    # Function count by name
    table = PrettyTable()
    header = ["Function name"]
    for r in range(tr.procs): header.append("Rank "+str(r))
    table.field_names = header
    for func in functionCounter:
        table.add_row([func] + functionCounter[func])
    print(table)
    html.functionTable = table.get_html_string()

    # Function count by type (MPI/POSIX/HDF5)
    mpi, posix, hdf5 = 0, 0, 0
    for func in functionCounter:
        if func.startswith("MPI"): mpi += sum(functionCounter[func])
        elif func.startswith("H5"): hdf5 += sum(functionCounter[func])
        else: posix += sum(functionCounter[func])
    html.functionCountImage = "./figures/function_categories.jpg"
    draw_pie_chart(["MPI", "POSIX", "HDF5"], [mpi, posix, hdf5], title="Function Counts", save_to=html.functionCountImage)

    # Function access bytes
    html.ioSizesImage=  "./figures/io_access_sizes.jpg"
    df = tr.get_posix_io()
    print(df['count'].describe())   #TODO: put into html
    x = range(0, 128*11, 128)
    y = []
    total = df.shape[0]
    for count in x:
        y.append( (df[df['count'] < count].shape[0]) / total )
    draw_bar_chart(x, y, title="IO Access Sizes", save_to=html.ioSizesImage, ylabel="Percentage", xlabel="Access size")


def offset_statistics(tr: TraceReader, html: HTMLWriter):
    html.offsetVsRankImage = "./figures/offset_vs_rank.jpg"
    draw_offset_vs_rank(tr.get_posix_io(), save_to =html.offsetVsRankImage)

if __name__ == "__main__":
    html = HTMLWriter("./simple_report.html")
    tr = TraceReader(sys.argv[1])
    file_statistics(tr, html)
    function_statistics(tr, html)
    offset_statistics(tr, html)
    html.write_html()
