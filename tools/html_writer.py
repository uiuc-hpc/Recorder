#!/usr/bin/env python
# encoding: utf-8

from xhtml2pdf import pisa

css_style = """
<style>
  table {
    border-collapse: collapse;
  }
  th, td {
    border: 1px solid #ccc;
    padding: 8px;
    text-align: left;
  }
  tr:nth-child(even) {
    background-color: #eee;
  }
  tr:nth-child(odd) {
    background-color: #fff;
  }
</style>
"""

class HTMLWriter:

    def __init__(self, path):
        self.path = path
        self.fileTable = ""
        self.fileAccessModeTable = ""
        self.fileSizeImage = ""
        self.functionTable = ""
        self.functionCountImage = ""
        self.offsetVsRankImage = ""
        self.accessPatternTable = ""
        self.ioSizesImage = ""

    def write_html(self):
        html_content  = """
        <html>
            <head> %s </head>
            <body>
                <h2> 1. File Statistics </h2>
                <h4> 1.1 Number of file access by each rank</h4>
                %s
                <h4> 1.2 Access mode of each file </h4>
                %s
                <h4>1.3 File Sizes</h4>
                <img src="%s" alt="File Size" width="500"></img>

                <h2> 2. Function Statistics </h3>
                <h4> 2.1 Funciton classifications </h4>
                <img src="%s" alt="Function Count" width="500"></img>
                <h4> 2.2 Fcount count </h4>
                %s

                <h2> 3. Offset VS Rank </h2>
                <img src="%s" alt="offset vs rank" width="600"></img>
                %s

                <h2> 4. Percentage of I/O access sizes </h2>
                <img src="%s" alt="access sizes" width="500"></img>
            </body>
        </html>
        """ %(css_style, self.fileTable, self.fileAccessModeTable, self.fileSizeImage,    \
                self.functionCountImage, self.functionTable,    \
                self.offsetVsRankImage, self.accessPatternTable, self.ioSizesImage)

        f = open("./simple_report.html", "w")
        f.write(html_content)
        f.close()

        self.write_pdf(html_content)


    def write_pdf(self, html_content):
        f = open("./simple_report.pdf", "w+b")
        status = pisa.CreatePDF(
            html_content,                # the HTML to convert
            dest=f)           # file handle to recieve result
        f.close()

