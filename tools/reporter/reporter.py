#!/usr/bin/env python
# encoding: utf-8
import sys
import recorder_viz
from recorder_viz import RecorderReader

if __name__ == "__main__":
    reader = RecorderReader(sys.argv[1])
    recorder_viz.generate_report(reader, "recorder-report.html")
