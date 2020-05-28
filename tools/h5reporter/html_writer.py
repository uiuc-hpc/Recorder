#!/usr/bin/env python
# encoding: utf-8
import os

def relpath(path):
    return os.path.relpath(path)

css_style = """
<style>
  html { font-size: 12px; }
  h2 { font-size: 14px; }
  h4 { font-size: 12px; }
  table {
    font-size: 12px;
    border-collapse: collapse;
  }
  th, td {
    border: 1px solid #ccc;
    padding: 4px;
    text-align: left;
  }
  tr:nth-child(even) {
    background-color: #eee;
  }
  tr:nth-child(odd) {
    background-color: #fff;
  }
  .content {
    max-width: 1000px;
    margin: auto;
  }
</style>
"""


class HTMLWriter:

    def __init__(self, path):
        self.path = path
        # 0
        self.performanceTable = ""
        self.recordCount = ""               # 0.1

        self.fileCount = ""                 # 1.1
        self.fileAccessModeTable = ""       # 1.2

        # 2.
        self.functionLayers = ""            # 2.1
        self.functionPatterns = ""           # 2.2
        self.functionCount = ""             # 2.3

        # 3.
        self.overallIOActivities = ""
        self.offsetVsRank = ""
        self.offsetVsTime = ""
        self.fileAccessPatterns = ""

        # 4.
        self.ioSizes = ""

    def write_html(self):
        html_content  = """
        <html>
            <head>
                %s
                <script src="https://cdn.bokeh.org/bokeh/release/bokeh-1.4.0.min.js"></script>
                <script src="https://cdn.bokeh.org/bokeh/release/bokeh-widgets-1.4.0.min.js"></script>
                <script src="https://cdn.bokeh.org/bokeh/release/bokeh-tables-1.4.0.min.js"></script>
            </head>
            <body><div class="content">
                <h2> 0. Performance </h2>
                %s
                <h4> 0.1 Record Count </h4>
                %s
                <h2> 1. File Statistics </h2>
                <h4> 1.1 Number of file accessed by each rank</h4>
                %s
                <!--
                <h4> 1.2 Access mode of each file </h4>
                <div style="height:400px; overflow:auto;">
                %s
                </div>
                -->
                <hr>

                <h2> 2. Function Statistics </h3>
                <div>
                    <div style="display:inline-block">
                        <h4> 2.1 I/O Layers</h4>
                        %s
                    </div>
                    <div style="display:inline-block">
                        <h4> 2.2 POSIX I/O Patterns </h4>
                        %s
                    </div>
                </div>
                <h4> 2.3 Function count </h4>
                %s
                <hr>


                <h2> 3. Access Patterns </h2>
                <h4> 3.1 Overall I/O activities </h4>
                %s
                <h4> 3.2 Accessed offsets VS ranks </h4>
                %s
                <h4> 3.3 Accessed offsets VS time </h4>
                %s
                <h4> 3.4 File access patterns </h4>
                <div style="height:400px; overflow:auto;">
                %s
                </div>
                <hr>

                <h2> 4. Count of I/O access sizes </h2>
                %s
            </div></body>
        </html>
        """ %(css_style, self.performanceTable, self.recordCount, self.fileCount, self.fileAccessModeTable, \
                self.functionLayers, self.functionPatterns, self.functionCount, \
                self.overallIOActivities, self.offsetVsRank, self.offsetVsTime, self.fileAccessPatterns, \
                self.ioSizes)

        f = open("./recorder-report.html", "w")
        f.write(html_content)
        f.close()

        #self.write_pdf(html_content)


    def write_pdf(self, html_content):
        from xhtml2pdf import pisa
        f = open("./reports.out/simple_report.pdf", "w+b")
        status = pisa.CreatePDF(
            html_content,                # the HTML to convert
            dest=f)           # file handle to recieve result
        f.close()
