|build|

Recorder 2.4
============

**A Multi-Level Library for Understanding I/O Activity in HPC
Applications**

We believe that multi-level I/O tracing and trace data analysis tool can
help end users understand the behavior of their application and I/O
subsystem, and can provide insights into the source of I/O performance
bottlenecks.

Recorder is a multi-level I/O tracing framework that can capture I/O
function calls at multiple levels of the I/O stack, including HDF5,
MPI-IO, and POSIX I/O. Recorder requires no modification or
recompilation of the application and users can control what levels are
traced.

.. toctree::
   build
   usage
   features
   changes
   


Usage
-----

Assume ``$RECORDER_ROOT`` is the location where you installed Recorder.

**1. Generate traces**

.. code:: bash

   # For MPI programs
   mpirun -np N -env LD_PRELOAD $RECORDER_ROOT/lib/librecorder.so ./your_app

   # For non-MPI programs or programs that may spwan non-mpi children programs
   RECORDER_WITH_NON_MPI=1 LD_PRELOAD=$RECORDER_ROOT/lib/librecorder.so ./your_app

mpirun can be changed to your workload manager, e.g. srun.

The trace files will be written to the current directory under a folder
named ``hostname-username-appname-pid-starttime``.

*Note: In some systems (e.g., Quartz at LLNL), Darshan is deployed
system-widely. Recorder does not work with Darshan. Please make sure
darhsn is disabled and your application is not linked with the darshan
library (use ldd to check).*

**2. Human-readable traces**

Recorder uses its own binary tracing format to compress and store
traces.

We provide a tool (recorder2text) that can convert the recorder format
traces to plain text format.

.. code:: bash

   $RECORDER_ROOT/bin/recorder2text /path/to/your_trace_folder/

This will generate text fomart traces under
``/path/to/your_trace_folder/_text``.

.. raw:: html

   <!---
   **3. Post-processing**

   We provide a Python library, [recorder-viz](https://pypi.org/project/recorder-viz/), for post-processing tasks.

   It can be used to automatically generate detailed visuazation reports, or can be used to directly access the traces information. 
   -->

Post-processing and Visualization
---------------------------------

**1. recorder-viz**

We developed a Python library,
`recorder-viz <https://github.com/wangvsa/recorder-viz>`__, for
post-processing and visualizations. Once installed, run the following
command to generate the visualization report.

.. code:: bash

   python $RECORDER_DIR/tools/reporter/reporter.py /path/to/your_trace_folder/

**2. Format Converters**

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

**3. C APIs**

TODO: we have C APIs (tools/reader.h). Need to doc them.

Dataset
-------

`Traces from 17 HPC applications <https://doi.org/10.6075/J0Z899X4>`__

The traces were collected using an old version of Recorder. The current
version uses a different trace format. To read those traces please use
Recorder 2.2.1 from the
`release <https://github.com/uiuc-hpc/Recorder/releases/tag/v2.2.1>`__
page.

Publications
------------

`Wang, Chen, Jinghan Sun, Marc Snir, Kathryn Mohror, and Elsa
Gonsiorowski. “Recorder 2.0: Efficient Parallel I/O Tracing and
Analysis.” In IEEE International Workshop on High-Performance Storage
(HPS), 2020. <https://doi.org/10.1109/IPDPSW50202.2020.00176>`__

`Wang, Chen, Kathryn Mohror, and Marc Snir. “File System Semantics
Requirements of HPC Applications.” Proceedings of the 30th International
Symposium on High-Performance Parallel and Distributed Computing (HPDC).
2021. <https://dl.acm.org/doi/abs/10.1145/3431379.3460637>`__

`Luu, Huong, Babak Behzad, Ruth Aydt, and Marianne Winslett. “A
multi-level approach for understanding I/O activity in HPC
applications.” In 2013 IEEE International Conference on Cluster
Computing (CLUSTER),
2013. <https://doi.org/10.1109/CLUSTER.2013.6702690>`__

.. |build| image:: https://github.com/uiuc-hpc/Recorder/actions/workflows/cmake.yml/badge.svg
   :target: https://github.com/uiuc-hpc/Recorder/actions/workflows/cmake.yml

