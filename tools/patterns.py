#!/usr/bin/env python
# encoding: utf-8
import sys
from bokeh.plotting import figure, output_file, show

current_id = 0
func_ids = {}

f = open(sys.argv[1], 'r')
lines = f.readlines()
f.close()


patterns = []
for line in lines:
    if "H5" not in line: continue
    func = line.split(' ')[2]
    if func not in func_ids:
        func_ids[func] = current_id
        current_id += 1

    patterns.append(func_ids[func])

#print(patterns)
print(len(patterns))

p = figure(plot_width=800, plot_height=400)
p.line(range(2000), patterns[0:2000], line_width=2)
show(p)

