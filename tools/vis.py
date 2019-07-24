#!/usr/bin/env python
# encoding: utf-8
import matplotlib
import matplotlib.patches as mpatches
from matplotlib.collections import PatchCollection
import matplotlib.pyplot as plt
import pandas as pd
import math
import numpy as np

# plot the cumulative histogram
# xs: a list of list
def draw_hist_chart(xs, labels, nbins=50, title="", xlabel="", ylabel="", save_to="/tmp/recorder_temp.png"):
    fig, ax = plt.subplots(1, len(xs), figsize=(10, 6))
    #fig, ax = plt.subplots()
    for index, x in enumerate(xs):
        if len(x)>0:
            ax[index].hist(x, nbins, density=True, histtype='step', cumulative=True, label=labels[index])
            #ax[index].grid(True)
            ax[index].legend(loc='best')
            ax[index].set_title(title)
            ax[index].set_xlabel(xlabel)
            ax[index].set_ylabel(ylabel)
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
x: categories, list of strings
y: value for each category, list of numbers
horizontal: draw the chart horizontally
'''
def draw_bar_chart(x:list, y:list, title="", save_to="/tmp/recorder_temp.png", horizontal=False, logScale=False, xlabel="", ylabel=""):
    fig, ax = plt.subplots()
    x_pos = np.arange(len(x))
    if horizontal:
        rects = ax.barh(x_pos, y, align='center', alpha=0.9, log=logScale)
        ax.set_yticks(x_pos)
        ax.set_yticklabels(x)
        ax.set_title(title)
    else:
        rects = ax.bar(x_pos, y, align='center', alpha=0.9)
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

def draw_overall_time_chart(df:pd.DataFrame, xlabel="", ylabel="", title="", save_to="/tmp/recorder_temp.png"):
    fig, ax = plt.subplots()

    yticks, yticklabels = [], []
    total_ranks = df['rank'].max()+1
    for rank in range(total_ranks):
        read_bars = df[(df['func'].str.contains("read")) & (df['rank']==rank)][['timestamp', 'duration']].values.tolist()
        write_bars = df[(df['func'].str.contains("write")) & (df['rank']==rank)][['timestamp', 'duration']].values.tolist()
        ax.broken_barh(read_bars, (rank, 1), facecolors="red")
        ax.broken_barh(write_bars, (rank, 1), facecolors="black")
        yticks.append(rank+0.5)
        yticklabels.append("rank "+str(rank))

    ax.grid(True)
    ax.set_yticks(yticks)
    ax.set_yticklabels(yticklabels)
    ax.set_xlabel("Time (seconds)")
    handles = []
    handles.append( mpatches.Patch(color="red", label="read") )
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

def offset_vs_rank_subplot(ax, bars, title):
    total_ranks = bars['rank'].max() + 1
    df = bars[bars['filename'] == title]

    for rank in range(total_ranks):
        read_df = df[(df['func'].str.contains('read')) & (df['rank']==rank)]
        write_df = df[(df['func'].str.contains('write')) & (df['rank']==rank)]

        read_bars = merge_bars(read_df)
        write_bars = merge_bars(write_df)

        ax.broken_barh(read_bars, (rank*2+0.5, 0.5), facecolors="gray")
        ax.broken_barh(write_bars, (rank*2, 0.5), facecolors="black")

    yticks, yticklabels = [], []
    for i in range(total_ranks):
        yticks.append(i * 2 + 0.5)
        yticklabels.append("rank "+str(i))
    ax.set_yticks(yticks)
    ax.set_yticklabels(yticklabels)
    ax.set_xlabel("File offset")

    ax.grid(True)
    ax.title.set_text(title.split("/")[-1])

def draw_offset_vs_rank(df:pd.DataFrame, save_to="/tmp/recorder_tmp.jpg"):
    filenames = list(set(df['filename']))

    rows = math.ceil(len(filenames) / 3)
    cols = min(len(filenames), 3)
    print("show chart: (%d, %d)" %(rows, cols))
    rows = min(4, rows)

    fig, ax = plt.subplots(rows, cols, constrained_layout=True, figsize=(10, 10/3*rows))
    for i in range(rows):
        for j in range(cols):
            index = i*cols + j
            if index < len(filenames):
                ax_ = ax
                if rows != 1 and cols != 1: ax_ = ax[i,j]
                elif rows == 1: ax_ = ax[j]
                else: ax_ = ax[i]
                offset_vs_rank_subplot(ax_, df, filenames[index])

    # Add legends
    handles = []
    #for i in range(4):
    handles.append( mpatches.Patch(color="gray", label="read") )
    handles.append( mpatches.Patch(color="black", label="write") )
    plt.legend(handles=handles)
    plt.savefig(save_to)


def offset_vs_time_subplot(ax, bars:pd.DataFrame, filename):
    colors = ['r', 'g', 'b', 'y']

    total_ranks = bars['rank'].max() + 1
    read_patches, write_patches = [], []
    write_line_x, write_line_y = [], []
    for i in range(total_ranks):
        read_patches.append([])
        write_patches.append([])
        write_line_x.append([])
        write_line_y.append([])

    df = bars[bars['filename'] == filename]
    records = df[['timestamp', 'duration', 'rank', 'func', 'offset', 'count']].values.tolist()
    for i in range(len(records)):
        timestamp, duration, rank, func, offset, count = records[i]

        # Remove all computation times and make the smallest I/O also visible
        if "write" in func:
            #if len(write_patches[rank]) > 0:  timestamp = write_patches[rank][-1].get_x() + write_patches[rank][-1].get_width()
            write_patches[rank].append(mpatches.Rectangle((timestamp, offset), duration, count))
            write_line_x[rank].append(timestamp)
            write_line_y[rank].append(offset)
        if "read" in func:
            #if len(read_patches[rank]) > 0: timestamp = read_patches[rank][-1].get_x() + read_patches[rank][-1].get_width()
            read_patches[rank].append(mpatches.Rectangle((timestamp, offset), duration, count))

    for rank in range(total_ranks):
        ax.add_collection(PatchCollection(read_patches[rank], facecolor=colors[rank], alpha=1.0))
        ax.add_collection(PatchCollection(write_patches[rank], facecolor=colors[rank], alpha=0.5))
        ax.plot(write_line_x[rank], write_line_y[rank], color=colors[rank], alpha=0.5)

    ax.set_ylabel("Offset")
    ax.set_xlabel("Time Flow")
    ax.autoscale_view()
    ax.grid(True)
    ax.title.set_text(filename.split("/")[-1])

def draw_offset_vs_time(df:pd.DataFrame, save_to="/tmp/recorder_tmp.jpg"):
    filenames = list(set(df['filename']))

    rows = math.ceil(len(filenames) / 3)
    cols = min(len(filenames), 3)
    print("offset vs time chart: (%d, %d)" %(rows, cols))
    rows = min(4, rows)

    fig, ax = plt.subplots(rows, cols, constrained_layout=True, figsize=(10, 10/3*rows))
    for i in range(rows):
        for j in range(cols):
            print(i, j)
            index = i*cols + j
            if index < len(filenames):
                ax_ = ax
                if rows != 1 and cols != 1: ax_ = ax[i,j]
                elif rows == 1: ax_ = ax[j]
                else: ax_ = ax[i]
                offset_vs_time_subplot(ax_, df, filenames[index])
    plt.savefig(save_to)

