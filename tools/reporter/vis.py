#!/usr/bin/env python
# encoding: utf-8

import sys, math
import numpy as np
from bokeh.plotting import figure, output_file, show
from bokeh.embed import components
from bokeh.io import export_png

from reader import RecorderReader, func_list
from html_writer import HTMLWriter

reader = RecorderReader(sys.argv[1])
htmlWriter = HTMLWriter("./")

def record_counts():
    y = []
    for meta in reader.localMetadata:
        y.append(meta.totalRecords)
    x = range(reader.globalMetadata.numRanks)
    p = figure(x_axis_label="Rank", y_axis_label="Number of records", plot_width=700, plot_height=500)

    p.xaxis.axis_label_text_font_style = "normal"
    p.yaxis.axis_label_text_font_style = "bold"
    p.xaxis.axis_label_text_font_size = "18pt"
    p.yaxis.axis_label_text_font_size = "18pt"
    p.xaxis.major_label_text_font_size = "18pt"
    p.yaxis.major_label_text_font_size = "18pt"
    from bokeh.models import NumeralTickFormatter
    p.yaxis.formatter=NumeralTickFormatter(format="0")

    p.vbar(x=x, top=y, width=0.6, bottom=1e5, color="grey")
    script, div = components(p)
    htmlWriter.recordCount = div+script

def function_counts():
    aggregate = np.zeros(256)
    for localMetadata in reader.localMetadata:
        aggregate += np.array(localMetadata.functionCounter)

    funcnames, counts = np.array([]), np.array([])
    for i in range(len(aggregate)):
        if aggregate[i] > 0:
            funcnames = np.append(funcnames, func_list[i].replace("PMPI", "MPI"))
            counts = np.append(counts, aggregate[i])

    index = np.argsort(counts)[::-1]
    counts = counts[index]
    funcnames = funcnames[index]

    print(np.sum(counts[0:10]) / np.sum(counts))

    p = figure(x_axis_label="Count", x_axis_type="log", y_axis_label="Function", plot_width=700, plot_height=500, y_range=funcnames[0:10])
    p.xaxis.axis_label_text_font_style = "normal"
    p.yaxis.axis_label_text_font_style = "bold"
    p.xaxis.axis_label_text_font_size = "18pt"
    p.yaxis.axis_label_text_font_size = "18pt"
    p.xaxis.major_label_text_font_size = "18pt"
    p.yaxis.major_label_text_font_size = "16pt"

    p.hbar(y=funcnames[0:10], right=counts[0:10], height=0.8, left=1e4, color="grey")
    script, div = components(p)
    htmlWriter.functionCount = div + script

if __name__ == "__main__":
    function_counts()
    record_counts()
    htmlWriter.write_html()
