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
cd Recorder
```

2. Compile Recorder with MPI and HDF5.

**Note that your application and Recorder must use the same version of HDF5 and MPI.**<br>

```console
./autogen.sh
./configure --prefix=[install location]
make && make install
```
By default, Recorde will trace function calls from all levels: HDF5, MPI and POSIX.
Options for `configure` can be used to disable one ore more levels of traces. Valid options:
 * --disable-posix
 * --disalbe-mpi
 * --disable-hdf5
 
If MPI or HDF5 is not located in standard paths, set CFLGAS and LDFLAGS to specify their location, e.g.,
```console
./configure --prefix=[install location] CFLAGS=-I/path/to/hdf5/include LDFLAGS=-L/path/to/hdf5/lib
```

3. Have fun.<br>
mpirun can be changed to your workload manager, e.g. srun.
```console
LD_PRELOAD=[install location]/lib/librecorder.so mpirun -np N ./your_app
```
The trace files will be written to the current directory.

Visualization
------------

We use Python libraries [pandas](https://pandas.pydata.org/), [bokeh](https://docs.bokeh.org/) and [prettytable](https://pypi.org/project/PrettyTable/) to generate the report.<br>
Please install those dependencies first.
Also, the visulation may take some time if there are a large number of I/O records.

```console
cd [install location]/bin/reporter
python reporter.py /path/to/your_trace_folder/
```
The visualization report (recorder-report.html) will be written into the current directory


Publication
-----------
[Wang, Chen, Jinghan Sun, Marc Snir, Kathryn Mohror, and Elsa Gonsiorowski. "Recorder 2.0: Efficient Parallel I/O Tracing and Analysis." In IEEE International Workshop on High-Performance Storage (HPS), 2020.](https://doi.org/10.1109/IPDPSW50202.2020.00176)

Luu, Huong, Babak Behzad, Ruth Aydt, and Marianne Winslett. "A multi-level approach for understanding I/O activity in HPC applications." In 2013 IEEE International Conference on Cluster Computing (CLUSTER), 2013.

Change Log
----------
**Recorder 2.1.5** Aug 27, 2020
1. Add MPI_File_seek and MPI_File_seek_shared
2. Add documentation on how to install using [Spack](https://spack.io).

**Recorder 2.1.4** Aug 26, 2020
1. Update LICENSE
2. Update automake/autotools files to make it ready for Spack

**Recorder 2.1.3** Aug 24, 2020
1. Use autotools and automake for compilation.
2. Add support for MPI_Comm_split/MPI_Comm_dup/MPI_Comm_create
3. Store the value of MPI_Status

**Recorder 2.1.2** Aug 06, 2020
1. Rewrite the reader program with C.
2. Add Python bindings to call C functions.
3. Add support for MPI_Waitall/Waitany/Waitsome and MPI_Ssend
4. Remove oft2 converter.
5. Clean up the Makefile.

**Recorder 2.1.1** Jun 28, 2020
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
