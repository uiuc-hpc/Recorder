#!/usr/bin/env python
# encoding: utf-8

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
  .content {
    max-width: 1000px;
    margin: auto;
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
        self.functionAccessTypeImage = ""
        self.functionCountImage = ""
        self.overallTimeChart = ""
        self.offsetVsRankImage = ""
        self.offsetVsTimeImage = ""
        self.accessPatternTable = ""
        self.ioSizesImage = ""

    def write_html(self):
        html_content  = """
        <html>
            <head> %s </head>
            <body><div class="content">
                <h2> 1. File Statistics </h2>
                <h4> 1.1 Number of file access by each rank</h4>
                %s
                <h4> 1.2 Access mode of each file </h4>
                %s
                <h4>1.3 File Sizes</h4>
                <img src="%s" alt="File Size" width="600"></img>
                <hr>

                <h2> 2. Function Statistics </h3>
                <div>
                    <div style="display:inline-block">
                        <h4> 2.1 I/O Layers</h4>
                        <img src="%s" alt="I/O Layers" height="320"></img>
                    </div>
                    <div style="display:inline-block">
                        <h4> 2.2 POSIX I/O Patterns </h4>
                        <img src="%s" alt="POSIX I/O Patterns" height="320"></img>
                    </div>
                </div>
                <h4> 2.3 Fcount count </h4>
                %s
                <hr>


                <h2> 3. Access Patterns </h2>
                <h4> 3.1 Overall I/O activities </h4>
                <img src="%s" alt="offset vs rank" width="500"></img>
                <h4> 3.2 Accessed offsets VS ranks </h4>
                <img src="%s" alt="offset vs rank" width="700"></img>
                <h4> 3.3 Accessed offsets VS time </h4>
                <img src="%s" alt="offset vs time" width="700"></img>
                <h4> 3.4 File access patterns </h4>
                %s
                <hr>

                <h2> 4. Percentage of I/O access sizes </h2>
                <img src="%s" alt="access sizes" width="700"></img>
            </div></body>
        </html>
        """ %(css_style, self.fileTable, self.fileAccessModeTable, self.fileSizeImage,    \
                self.functionCountImage, self.functionAccessTypeImage, self.functionTable,    \
                self.overallTimeChart, self.offsetVsRankImage, self.offsetVsTimeImage, self.accessPatternTable, \
                self.ioSizesImage)

        f = open("./reports.out/simple_report.html", "w")
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

