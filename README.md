Recorder
========
**A Multi-Level Library for Understanding I/O Activity in HPC Applications**

We believe that multi-level I/O tracing and trace data analysis tool can help
end users understand the behavior of their application and I/O subsystem, and
can provide insights into the source of I/O performance bottlenecks.

Recorder is a multi-level I/O tracing framework that can capture I/O function
calls at multiple levels of the I/O stack, including HDF5, MPI-IO, and POSIX
I/O. Recorder requires no modification or recompilation of the application and
users can control what levels are traced.


Publication
-----------

For a full description of the Recorder library please refer to the following
paper:

[A Multi-Level Approach for Understanding I/O Activity in HPC
Applications](http://web.engr.illinois.edu/~bbehza2/files/Babak_Behzad_IASDS_2013_paper.pdf)

Huong Vu Thanh Luu, Babak Behzad, Ruth Aydt, Marianne Winslett

Short Paper, Workshop on Interfaces and Abstractions for Scientific Data Storage
(IASDS 2013), in conjuction with IEEE Cluster 2013.


Description
-----------

We chose to build Recorder as a shared library so that it does not require
modification or recompilation of the application. Recorder uses function
interpositioning to prioritize itself over standard functions, as shown in the
Figure below. Once Recorder is specified as the preloading library, it
intercepts HDF5 function calls issued by the application and reroutes them to
the tracing implementation where the timestamp, function name, and function
parameters are recorded. The original HDF5 function is called after this
recording process. The mechanism is the same for the MPI and POSIX layers. ![alt
text](http://web.engr.illinois.edu/~bbehza2/files/H5Tuner-Design.png "Dynamic
instrumentation of I/O stack by Recorder")

Installation
------------

    ./config.sh /opt/hdf5-1.8.9 /opt/mpich2-1.4.1p1/ -DDISABLE_MPIO_TRACE -DDISABLE_POSIX_TRACE
    make
    make install prefix=${HOME}/librecorder


