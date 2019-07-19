Recorder v2.0
========
**A Multi-Level Library for Understanding I/O Activity in HPC Applications**

We believe that multi-level I/O tracing and trace data analysis tool can help
end users understand the behavior of their application and I/O subsystem, and
can provide insights into the source of I/O performance bottlenecks.

Recorder is a multi-level I/O tracing framework that can capture I/O function
calls at multiple levels of the I/O stack, including HDF5, MPI-IO, and POSIX
I/O. Recorder requires no modification or recompilation of the application and
users can control what levels are traced.





Description
-----------

We chose to build Recorder as a shared library so that it does not require
modification or recompilation of the application. Recorder uses function
interpositioning to prioritize itself over standard functions, as shown in the
Figure below. Once Recorder is specified as the preloading library, it
intercepts HDF5 function calls issued by the application and reroutes them to
the tracing implementation where the timestamp, function name, and function
parameters are recorded. The original HDF5 function is called after this
recording process. The mechanism is the same for the MPI and POSIX layers.

Installation & Usage
------------

    ./config.sh PATH_TO_HDF5 PATH_TO_MPI options
    // Options can be used to disable one ore more level of traces.
    // Valid options: -DDISABLE_HDF5_TRACE -DDISABLE_MPIO_TRACE -DDISABLE_POSIX_TRACE
    make
    make install prefix=${HOME}/librecorder
    LD_PRELOAD=/path/to/librecorder.so ./your_app


Publication
-----------

Luu, Huong, Babak Behzad, Ruth Aydt, and Marianne Winslett. "A multi-level approach for understanding I/O activity in HPC applications." In 2013 IEEE International Conference on Cluster Computing (CLUSTER), pp. 1-5. IEEE, 2013.

