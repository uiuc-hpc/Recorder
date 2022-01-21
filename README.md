[![build](https://github.com/uiuc-hpc/Recorder/actions/workflows/cmake.yml/badge.svg)](https://github.com/uiuc-hpc/Recorder/actions/workflows/cmake.yml)

# Recorder 2.3

**A Multi-Level Library for Understanding I/O Activity in HPC Applications**

We believe that multi-level I/O tracing and trace data analysis tool can help
end users understand the behavior of their application and I/O subsystem, and
can provide insights into the source of I/O performance bottlenecks.

Recorder is a multi-level I/O tracing framework that can capture I/O function
calls at multiple levels of the I/O stack, including HDF5, MPI-IO, and POSIX
I/O. Recorder requires no modification or recompilation of the application and
users can control what levels are traced.



## Building Recorder

There are two ways to build and install Recorder. The first way is to build Recorder from source using CMake. This is recomended as Recorder is currently under active development.

### 1. Building Recorder with CMake (recommended)

**Dependencies**

 - MPI
 - HDF5
 - Arrow (optional) > 5.0.0 - Needed for building Parquet convertor.
 - CUDA (optional) - Needed for CUDA kernels interception.

*Note that Recorder and the applications you intend to trace must be compiled with the same version of HDF5 and MPI.*

**Build & Install**

```bash
git clone https://github.com/uiuc-hpc/Recorder.git
cd Recorder
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=[install location]
make
make install
```
**CMake Options**

(1) Dependencies install location

If MPI, HDF5, or arrow is not installed in standard locations, you may need to use `-DCMAKE_PREFIX_PATH` to indicate their locations:
```bash
cmake ..                                                          \
      -DCMAKE_INSTALL_PREFIX=[install location]                   \
      -DCMAKE_PREFIX_PATH=[semicolon separated depedencies dir]
```

(2) Enable/disable tracing levels

By default, Recorde traces function calls from all levels: HDF5, MPI, MPI-IO and POSIX. The following options can be used to enable/disable specific levels.
 * -DRECORDER_ENABLE_POSIX_TRACE=[ON|FF]
 * -DRECORDER_ENABLE_MPI_TRACE=[ON|FF]
 * -DRECORDER_ENABLE_MPIIO_TRACE=[ON|FF]
 * -DRECORDER_ENABLE_HDF5_TRACE=[ON|FF]

(3) Intercepting `fcntl()` call: 

Since v2.1.7, `fcntl(int fd, int cmd, ...)` is intercepted. The commands (2nd argument) defined in POSIX standard
are supported. If non-POSIX commands were used, please disable fcntl tracing at configure time with `-DRECORDER_ENABLE_FCNTL_TRACE=OFF`.

(4) Intercepting CUDA kernels:

add `-DRECORDER_ENABLE_CUDA_TRACE=ON` to cmake to allow tracing CUDA kernels.

(5) Parquet Converter

add `-DRECORDER_ENABLE_PARQUET=ON` to cmake to build the Parquet format converter




**Features Controled by Environment Variables**

(1) Inclusion/Exclusion prefix list

Many POSIX calls intercepted are made by system libraries, job schedulers, etc. Their I/O accesses are less interesting as they operates on
file locations such as `/dev, /sys, /usr/lib, etc`. To ignore those calls, you can specifiy a file that contains prefixes that you want to
exclude:

```bash
export RECORDER_EXCLUSION_FILE=/path/to/your/exclusion_prefix_file

# This file contains one prefix each line.
# An example exclusion prefix file is included with Recorder:

Recorder$ cat ./exclusion_prefix.txt 
/dev
/proc
/sys
/etc
/usr/tce/packages
pipe:[
anon_inode:
socket:[
```

Similarly, you can set `RECORDER_INCLUSION_FILE` to specify the inclusion prefixes, so only the POSIX calls that match those prefixes will be recorded.

Note that this feature only applies to POSIX calls. MPI and HDF5 calls are always recorded when enabled.

(2) Storing pointers

Recorder by default does not log the pointers (memory addresses) as they provide little information yet
cost a lot of space to store.
However, you can change this behaviour by setting the enviroment variable `RECORDER_LOG_POINTER` to 1.

(3) Location to write traces:

By default Recorder will output the traces to the current working directory.
You can use the enviroment variable `RECORDER_TRACES_DIR` to specifiy the path where you want the traces stored.
Make sure that every process has the persmission to write to that directory. 

(4) Buffer size

Timestamps are buffered internally to avoid frequent disk I/O. Use `RECORDER_BUFFER_SIZE` (in MB) to set 
the size of this buffer. The default value is 1MB.


### 2. Building Recorder with Spack

*NOTE: please do not use Spack to install Recorder for now. The version there is outdated, we will update it soon.*

For now, building Recorder with Spack provides less flexibility. We will add the CMake options for spack as well.

```bash
spack install recorder
```
By default Recorder generates traces from all levels, you can use **~** to disable a specific level.

E.g., the following command will install Recorder with HDF5 and MPI tracing disabled.
```bash
spack install recorder~hdf5~mpi
```

## Usage

Assume `$RECORDER_ROOT` is the location where you installed Recorder.

**1. Generate traces**

```bash
# For MPI programs
mpirun -np N -env LD_PRELOAD $RECORDER_ROOT/lib/librecorder.so ./your_app

# For non-MPI programs or programs that may spwan non-mpi children programs
RECORDER_WITH_NON_MPI=1 LD_PRELOAD=$RECORDER_ROOT/lib/librecorder.so ./your_app
```
mpirun can be changed to your workload manager, e.g. srun.

The trace files will be written to the current directory under a folder named `hostname-username-appname-pid-starttime`. 

*Note: In some systems (e.g., Quartz at LLNL), Darshan is deployed system-widely. Recorder does not work with Darshan. Please make sure darhsn is disabled and your application is not linked with the darshan library (use ldd to check).*

**2. Human-readable traces**

Recorder uses its own binary tracing format to compress and store traces.

We provide a tool (recorder2text) that can convert the recorder format traces to plain text format.
```bash
$RECORDER_ROOT/bin/recorder2text /path/to/your_trace_folder/
```
This will generate text fomart traces under `/path/to/your_trace_folder/_text`.

<!---
**3. Post-processing**

We provide a Python library, [recorder-viz](https://pypi.org/project/recorder-viz/), for post-processing tasks.

It can be used to automatically generate detailed visuazation reports, or can be used to directly access the traces information. 
-->


## Post-processing and Visualization

**1. recorder-viz**

We developed a Python library, [recorder-viz](https://github.com/wangvsa/recorder-viz), for post-processing and visualizations.
Once installed, run the following command to generate the visualization report.
```bash
python $RECORDER_DIR/tools/reporter/reporter.py /path/to/your_trace_folder/
```
**2. Format Converters**

We also provide two format converters `recorder2parquet` and `recorder2timeline`. They will be placed under $RECORDER_ROOT/bin directory after installation.

- `recorder2parquet` will convert Recorder traces into a single [Parquet](https://parquet.apache.org) formata file. The Apache Parquet format is a well-known format that is supported by many analysis tools.

- `recorder2timeline` will conver Recorder traces into [Chromium](https://www.chromium.org/developers/how-tos/trace-event-profiling-tool/trace-event-reading) trace format files. You can upload them to [https://ui.perfetto.dev](https://ui.perfetto.dev) for an interactive visualization.

**3. C APIs**

TODO: we have C APIs (tools/reader.h). Need to doc them.


## Dataset

[Traces from 17 HPC applications](https://doi.org/10.6075/J0Z899X4)

The traces were collected using an old version of Recorder. The current version uses a different trace format.
To read those traces please use Recorder 2.2.1 from the [release](https://github.com/uiuc-hpc/Recorder/releases/tag/v2.2.1) page.


Publications
-----------
[Wang, Chen, Kathryn Mohror, and Marc Snir. "File System Semantics Requirements of HPC Applications." Proceedings of the 30th International Symposium on High-Performance Parallel and Distributed Computing (HPDC). 2021.](https://dl.acm.org/doi/abs/10.1145/3431379.3460637)

[Wang, Chen, Jinghan Sun, Marc Snir, Kathryn Mohror, and Elsa Gonsiorowski. "Recorder 2.0: Efficient Parallel I/O Tracing and Analysis." In IEEE International Workshop on High-Performance Storage (HPS), 2020.](https://doi.org/10.1109/IPDPSW50202.2020.00176)

[Luu, Huong, Babak Behzad, Ruth Aydt, and Marianne Winslett. "A multi-level approach for understanding I/O activity in HPC applications." In 2013 IEEE International Conference on Cluster Computing (CLUSTER), 2013.](https://doi.org/10.1109/CLUSTER.2013.6702690)



Change Log
----------

**Recorder 2.3.3** Jan 21, 2022
1. Still require a RECORDER\_WITH\_NON\_MPI hint for non-mpi programs.
2. Add a singal handler to intercept SIGTERM and SIGINT.
3. Allow setting buffer size

**Recorder 2.3.2** Jan 18, 2022
1. Can handle both MPI and non-MPI programs without user hint.
2. Can handle fork() + exec() workflows.

**Recorder 2.3.1** Nov 30, 2021
1. Separate MPI and MPI-IO.
2. Updated conflict detector to use the latest reader code.

**Recorder 2.3.0** Sep 15, 2021
1. Adopt pilgrim copmerssion algorithm.
2. Implemented a new reader and decorder interface.
3. Support GNU ftrace functionality.
4. Support multi-threaded programs. Record has a thread id field.
5. Store records in starting timestamp order.
6. Store level information for each record.
7. Add APIs to retrive function type and function name, etc.

**Recorder 2.2.1** Aug 25, 2021
1. Include the code for verifying I/O synchronizations (tools/verifyio).
2. Add support for multi-threaded programs.

**Recorder 2.2.0** Jan 25, 2021
1. Add support for MPI_Cart_sub, MPI_Comm_split_type, etc.
2. Assign each MPI_Comm object a globally unique id.

**Recorder 2.1.9** Jan 14, 2021
1. Clean up the code
2. Fixed a few memory leak issues
3. Add support for fseeko and ftello

**Recorder 2.1.8** Dec 18, 2020
1. Add MPI_Test, MPI_Testany, MPI_Testsome and MPI_Testall
2. Add MPI_Ireduce, MPI_Iscatter, MPI_Igather and MPI_Ialltoall
3. Do not log pointers by default as it delivers not so much information

**Recorder 2.1.7** Nov 11, 2020
1. Add fcntl() support. Only support commands defined in [POSIX standard](https://pubs.opengroup.org/onlinepubs/009695399/functions/fcntl.html).
2. Add support for MPI_Ibcast()

**Recorder 2.1.6** Nov 05, 2020
1. Generate unique id for communicators 
2. Fix bus caused by MPI_COMM_NULL
3. Add support for MPI_File_get_size

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
