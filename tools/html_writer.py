#!/usr/bin/env python
# encoding: utf-8

class HTMLWriter:

    def __init__(self, path):
        self.path = path
        self.fileTable = ""
        self.fileSizeImage = ""
        self.functionTable = ""
        self.functionCountImage = ""


    def write_html(self):
        html = """
        <html>
            <head></head>
            <body>
                <h3> 1. File Statistics </h3>
                %s </br>
                <img src="%s" alt="File Size"></img>
                <h3> 2. Function Statistics </h3>
                <img src="%s" alt="Function Count"></img>
                %s
            </body>
        </html>
        """ %(self.fileTable, self.fileSizeImage, self.functionCountImage, self.functionTable)
        f = open("./simple_report.html", "w")
        f.write(html)
        f.close()
