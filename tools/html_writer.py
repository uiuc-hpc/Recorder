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
        self.fileSizeImage = ""
        self.functionTable = ""
        self.functionCountImage = ""
        self.offsetVsRankImage = ""
        self.ioSizesImage = ""

    def write_html(self):
        html_content  = """
        <html>
            <head> %s </head>
            <body>
                <h3> 1. File Statistics </h3>
                %s </br>
                <img src="%s" alt="File Size" width="500"></img>
                <h3> 2. Function Statistics </h3>
                <img src="%s" alt="Function Count" width="500"></img>
                %s
                <h3> 3. Offset VS Rank </h3>
                <img src="%s" alt="offset vs rank" width="600"></img>
                <h4> 4. Histogram of access sizes </h4>
                <img src="%s" alt="histogram of access sizes" width="500"></img>
            </body>
        </html>
        """ %(css_style, self.fileTable, self.fileSizeImage,    \
                self.functionCountImage, self.functionTable,    \
                self.offsetVsRankImage, self.ioSizesImage)

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

