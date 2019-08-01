#!/usr/bin/env python
# encoding: utf-8
import matplotlib
import matplotlib.patches as mpatches
from matplotlib.collections import PatchCollection
import matplotlib.pyplot as plt
from reader import TraceReader
import pandas as pd

import math
import numpy as np

# plot the cumulative histogram
# xs: a list of list
def draw_multi_bar_chart(xs, ys, titles=[""], xlabel="", ylabel="", logScale=False, save_to="/tmp/recorder_temp.png"):
    figsize = (10, 6) if len(xs[0])+len(xs[1]) < 30 else (15,6)
    fig, axes = plt.subplots(1, len(xs), figsize=figsize)
    for index, x in enumerate(xs):
        if len(x)>0:
            ax = axes[index]
            y = ys[index]
            x_pos = np.arange(len(x))
            rects = ax.bar(x_pos, y, align='center', alpha=0.9, log=logScale)
            ax.set_xticks(x_pos)
            if len(x) > 10:
                ax.xaxis.set_tick_params(rotation=90)
            ax.set_xticklabels(x)
            for i, v in enumerate(y):
                text = "%.2f"%(v) if isinstance(v, float) else str(v)
                ax.text(i-0.15, v, text, fontweight='bold')
            ax.set_xlabel(xlabel)
            ax.set_ylabel(ylabel)
            ax.set_title(titles[index])
    fig.tight_layout()
    fig.savefig(save_to)


def draw_pie_chart(x, y, save_to="/tmp/recorder_tmp.jpg"):
    fig, ax = plt.subplots()
    explode = [0] * len(x)
    explode[0] =  0.1
    wedges, texts, autotexts = ax.pie(y, explode=explode, shadow=True, autopct='%1.1f%%')
    ax.axis('equal')    # Equal aspect ratio ensures that pie is drawn as a circle.
    ax.legend(wedges, x)
    fig.tight_layout()
    fig.savefig(save_to, bbox_inches='tight', pad_inches=0)

'''
Draw a single bar chart
x: list of strings, where each entry a category
y: list of numbers - value for each category
horizontal: draw the chart horizontally
'''
def draw_bar_chart(x:list, y:list, title="", save_to="/tmp/recorder_temp.png", horizontal=False, logScale=False, xlabel="", ylabel=""):
    fig, ax = plt.subplots()
    x_pos = np.arange(len(x))
    if horizontal:
        fig.set_size_inches(7, max(0.2*len(x), 4))
        rects = ax.barh(x_pos, y, align='center', alpha=0.9, log=logScale)
        ax.set_yticks(x_pos)
        ax.set_yticklabels(x)
        ax.set_title(title)
        ax.set_ylim(min(x_pos)-1, max(x_pos)+1)
    else:
        rects = ax.bar(x_pos, y, align='center', alpha=0.9, log=logScale)
        ax.set_xticks(x_pos)
        ax.set_xticklabels(x)
        ax.set_title(title)

    if horizontal:
        for i, v in enumerate(y):
            text = "%.2f"%(v) if isinstance(v, float) else str(v)
            ax.text(v, i, text, fontweight='bold')
    else:
        for i, v in enumerate(y):
            text = "%.2f"%(v) if isinstance(v, float) else str(v)
            ax.text(i-0.15, v, text, fontweight='bold')

    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    fig.tight_layout()
    plt.savefig(save_to)

def draw_overall_time_chart(tr:TraceReader, xlabel="", ylabel="", title="", save_to="/tmp/recorder_temp.png"):
    df = tr.get_posix_io()
    fig, ax = plt.subplots(figsize=(10,max(4, 0.2*tr.procs)))

    yticks, yticklabels = [], []
    for rank in range(tr.procs):
        #read_bars = df[(df['func'].str.contains("read")) & (df['rank']==rank)][['timestamp', 'duration']].values.tolist()
        #write_bars = df[(df['func'].str.contains("write")) & (df['rank']==rank)][['timestamp', 'duration']].values.tolist()
        #ax.broken_barh(read_bars, (rank, 1), facecolors="red")
        #ax.broken_barh(write_bars, (rank, 1), facecolors="black")
        yticks.append(rank)
        yticklabels.append("rank "+str(rank))

    write_ops = df[(df['func'].str.contains("write"))][['timestamp', 'rank']].values.tolist()
    write_bar_x, write_bar_y = [], []
    for op in write_ops:
        write_bar_x.append(op[0])
        write_bar_y.append(op[1])
    read_ops = df[(df['func'].str.contains("read"))][['timestamp', 'rank']].values.tolist()
    read_bar_x, read_bar_y = [], []
    for op in read_ops:
        read_bar_x.append(op[0])
        read_bar_y.append(op[1])

    ax.scatter(write_bar_x, write_bar_y, alpha=1.0, marker=".", s=20, c="black")
    ax.scatter(read_bar_x, read_bar_y, alpha=1.0, marker="x", s=20, c="gray")

    ax.set_xlim(0, tr.end_time-tr.start_time)
    ax.grid(True)
    ax.set_yticks(yticks)
    ax.set_yticklabels(yticklabels)
    ax.set_xlabel("Time (seconds)")
    handles = []
    handles.append( mpatches.Patch(color="gray", label="read") )
    handles.append( mpatches.Patch(color="black", label="write") )
    fig.tight_layout()
    plt.legend(handles=handles)
    plt.savefig(save_to)


# O(nlogn) to merge the overlapping/contiguous intervals
def merge_bars(df:pd.DataFrame):
    mergedBars = []
    if df.shape[0] == 0: return mergedBars

    bars = df[['offset', 'count']].values.tolist()
    from operator import itemgetter
    bars = sorted(bars, key=itemgetter(0))    # sort by starting position

    start, mergedCount = bars[0][0], bars[0][1]
    for i in range(1, len(bars)):
        if start+mergedCount >= bars[i][0]: # overlap intervals
            mergedCount = max(start+mergedCount, sum(bars[i])) - start
        else:
            mergedBars.append((start, mergedCount))
            start, mergedCount = bars[i][0], bars[i][1]
    mergedBars.append((start, mergedCount))
    return mergedBars

def offset_vs_rank_subplot(ax, tr:TraceReader, filename):
    df = tr.get_posix_io()
    df = df[df['filename'] == filename]

    for rank in range(tr.procs):
        read_df = df[(df['func'].str.contains('read')) & (df['rank']==rank)]
        write_df = df[(df['func'].str.contains('write')) & (df['rank']==rank)]

        read_bars = merge_bars(read_df)
        write_bars = merge_bars(write_df)

        ax.broken_barh(read_bars, (rank*2+0.5, 0.5), facecolors="gray")
        ax.broken_barh(write_bars, (rank*2, 0.5), facecolors="black")

    yticks, yticklabels = [], []
    for i in range(tr.procs):
        yticks.append(i * 2 + 0.5)
        yticklabels.append("rank "+str(i))
    ax.set_yticks(yticks)
    ax.set_yticklabels(yticklabels)
    ax.set_xlabel("File offset")

    ax.grid(True)
    ax.title.set_text(filename.split("/")[-1])

def draw_offset_vs_rank(tr:TraceReader, save_to="/tmp/recorder_tmp.jpg"):

    rows = math.ceil(len(tr.files) / 4)
    cols = min(len(tr.files), 4)
    print("show chart: (%d, %d)" %(rows, cols))
    rows = min(4, rows)

    height = max(0.2*tr.procs*rows, 3.3*rows) # at least 3.3 inch height per row
    fig, ax = plt.subplots(rows, cols, constrained_layout=True, figsize=(14, height))
    for i in range(rows):
        for j in range(cols):
            index = i*cols + j
            if index < len(tr.files):
                ax_ = ax
                if rows != 1 and cols != 1: ax_ = ax[i,j]
                elif rows == 1: ax_ = ax[j]
                else: ax_ = ax[i]
                offset_vs_rank_subplot(ax_, tr, tr.files[index])

    # Add legends
    handles = []
    #for i in range(4):
    handles.append( mpatches.Patch(color="gray", label="read") )
    handles.append( mpatches.Patch(color="black", label="write") )
    plt.legend(handles=handles)
    plt.savefig(save_to)


colors = ['r', 'g', 'b', 'y']
def offset_vs_time_subplot(ax, tr:TraceReader, filename):
    def get_cmap(n, name='hsv'):
        '''Returns a function that maps each index in 0, 1, ..., n-1 to a distinct
        RGB color; the keyword argument name must be a standard mpl colormap name.'''
        return plt.cm.get_cmap(name, n)


    write_dots_x, write_dots_y, read_dots_x, read_dots_y = [], [], [], []
    read_patches, write_patches = [], []
    for i in range(tr.procs):
        read_patches.append([])
        write_patches.append([])
        write_dots_x.append([])
        write_dots_y.append([])
        read_dots_x.append([])
        read_dots_y.append([])

    df = tr.get_posix_io()
    df = df[df['filename'] == filename]
    records = df[['timestamp', 'duration', 'rank', 'func', 'offset', 'count']].values.tolist()
    for i in range(len(records)):
        timestamp, duration, rank, func, offset, count = records[i]

        # Remove all computation times and make the smallest I/O also visible
        if "write" in func:
            write_patches[rank].append(mpatches.Rectangle((timestamp, offset), duration, count))
            write_dots_x[rank].append(timestamp)
            write_dots_y[rank].append(offset)
        if "read" in func:
            read_patches[rank].append(mpatches.Rectangle((timestamp, offset), duration, count))
            read_dots_x[rank].append(timestamp)
            read_dots_y[rank].append(offset)

    cmap = plt.cm.get_cmap("hsv", tr.procs+1)
    for rank in range(tr.procs):
        ax.add_collection(PatchCollection(read_patches[rank], facecolor=cmap(rank), alpha=1.0))
        ax.add_collection(PatchCollection(write_patches[rank], facecolor=cmap(rank), alpha=0.5))
        ax.scatter(read_dots_x[rank], read_dots_y[rank], c=cmap(rank), alpha=1.0, s=2)
        ax.scatter(write_dots_x[rank], write_dots_y[rank], c=cmap(rank), alpha=0.5, s=2)

    ax.set_ylabel("Offset")
    ax.set_xlabel("Time")
    ax.autoscale_view()
    ax.grid(True)
    ax.title.set_text(filename.split("/")[-1])

def draw_offset_vs_time(tr:TraceReader, save_to="/tmp/recorder_tmp.jpg"):
    rows = math.ceil(len(tr.files) / 4)
    cols = min(len(tr.files), 4)
    print("offset vs time chart: (%d, %d)" %(rows, cols))
    rows = min(4, rows)

    fig, ax = plt.subplots(rows, cols, constrained_layout=True, figsize=(14, 3.3*rows))
    for i in range(rows):
        for j in range(cols):
            print(i, j)
            index = i*cols + j
            if index < len(tr.files):
                ax_ = ax
                if rows != 1 and cols != 1: ax_ = ax[i,j]
                elif rows == 1: ax_ = ax[j]
                else: ax_ = ax[i]
                offset_vs_time_subplot(ax_, tr, tr.files[index])

    handles = []
    for i, c in enumerate(colors):
        handles.append(mpatches.Patch(color=c, label='rank '+str(i) ))
    plt.legend(handles=handles)
    plt.savefig(save_to)

