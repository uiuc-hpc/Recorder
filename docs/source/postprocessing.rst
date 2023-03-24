Post-processing and Visualization
=================================

1. recorder-viz
---------------

We developed a Python library,
`recorder-viz <https://github.com/wangvsa/recorder-viz>`__, for
post-processing and visualizations. Once installed, run the following
command to generate the visualization report.

.. code:: bash

   python $RECORDER_DIR/tools/reporter/reporter.py /path/to/your_trace_folder/

2. Format Converters
--------------------

We also provide two format converters ``recorder2parquet`` and
``recorder2timeline``. They will be placed under $RECORDER_ROOT/bin
directory after installation.

-  ``recorder2parquet`` will convert Recorder traces into a single
   `Parquet <https://parquet.apache.org>`__ formata file. The Apache
   Parquet format is a well-known format that is supported by many
   analysis tools.

-  ``recorder2timeline`` will conver Recorder traces into
   `Chromium <https://www.chromium.org/developers/how-tos/trace-event-profiling-tool/trace-event-reading>`__
   trace format files. You can upload them to https://ui.perfetto.dev
   for an interactive visualization.

3. APIs
---------

TODO: we have C APIs (tools/reader.h). Need to doc them.

