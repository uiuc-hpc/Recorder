#!/usr/bin/env python
# encoding: utf-8
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import pandas as pd
import math
import numpy as np

def draw_pie_chart(x, y, title, save_to="/tmp/recorder_tmp.jpg"):
    fig, ax = plt.subplots()
    explode = [0] * len(x)
    explode[0] =  0.1
    ax.pie(y, labels=x, explode=explode, shadow=True, autopct='%1.1f%%')
    ax.axis('equal')    # Equal aspect ratio ensures that pie is drawn as a circle.
    fig.savefig(save_to)


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


def merge_bars(df:pd.DataFrame):
    mergedBars = []
    if df.shape[0] == 0:
        return mergedBars

    bars = df[['offset', 'count']].values.tolist()

    i = 0
    while i < len(bars):
        op1 = bars[i]
        i = i + 1
        mergedCount = op1[1]
        for j in range(i, len(bars)):
            op2 = bars[j]
            if op1[0]+op1[1] == op2[0]:
                i = j + 1
                mergedCount += op1[1]
            else:
                break

        mergedBars.append((op1[0], mergedCount))
    return mergedBars


def offset_vs_rank_subplot(ax, bars, title):
    total_ranks = bars['rank'].max() + 1
    df = bars[bars['filename'] == title]

    # Combine operations, i.e. merge those contiguous I/Os
    # to reduce the rendering time

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
    print(filenames, len(filenames))

    rows = math.ceil(len(filenames) / 3)
    cols = min(len(filenames), 3)
    print("show chart: (%d, %d)" %(rows, cols))

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
