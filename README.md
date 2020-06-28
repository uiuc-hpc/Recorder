Recorder 2.1
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

1. Get the code.

```console
git clone https://github.com/uiuc-hpc/Recorder.git
```

2. Tell Recorder where to find HDF5 and MPI.

**Note that your application and Recorder must use the same version of HDF5 and MPI.**<br>
Options can be used to disable one ore more level of traces. Valid options:
 * -DDISABLE_HDF5_TRACE
 * -DDISABLE_MPIO_TRACE
 * -DDISABLE_POSIX_TRACE
```console
./config.sh PATH_TO_HDF5 PATH_TO_MPI [options]
```

3. Make and install.
```console
make
make install prefix=$HOME
```

4. Have fun.<br>
mpirun can be changed to your workload manager, e.g. srun.
```console
LD_PRELOAD=/path/to/librecorder.so mpirun -np N ./your_app
```
The trace files will be written to the current directory.

Visualization
------------

We use Python libraries [pandas](https://pandas.pydata.org/), [bokeh](https://docs.bokeh.org/) and [prettytable](https://pypi.org/project/PrettyTable/) to generate the report.<br>
Please install those dependencies first.
Also, the visulation may take some time if the number of I/O records is large.

```console
cd /path/to/Recorder/tools/reporter
python reporter.py /path/to/your_trace_folder/
```
The visualization report (recorder-report.html) will be written into the current directory


Publication
-----------
Wang, Chen, Jinghan Sun, Marc Snir, Kathryn Mohror, and Elsa Gonsiorowski. "Recorder 2.0: Efficient Parallel I/O Tracing and Analysis." In IEEE International Workshop on High-Performance Storage (HPS), 2020.

Luu, Huong, Babak Behzad, Ruth Aydt, and Marianne Winslett. "A multi-level approach for understanding I/O activity in HPC applications." In 2013 IEEE International Conference on Cluster Computing (CLUSTER), 2013.

Change Log
----------
**Recorder 2.1.1** June 28, 2020
1. Use [uthash](https://github.com/troydhanson/uthash) library to replace the original hash map implementation
2. Remove zlib support

**Recorder 2.1** May 15, 2020
1. Dump a VERSION file for the reader script to decide the trace format.
2. Include the return value in each record.
3. Implement conflict detection algorithm for commit semantics and session semantics.

**Recorder 2.0.1** Nov 7, 2019
1. Implement compressed peephole encoding schema.
2. Intergrat zlib as another compression choice.
3. Users can choose compression modes by setting environment variables.

**Recorder 2.0** Jul 19, 2019
1. Add the binary format output.
2. Implement a converter that can output OTF2 trace format.
3. Write a separate  log unit to provide an uniform logging interface. Re-write most of the code to use this new log unit.
4. Ignore files (e.g. /sockets) that are not used by the application itself.
5. Add a built-in hashmap to support mappings from function name and filename to integers.
6. Put all function (that we plan to intercept) signatures in the same header file
