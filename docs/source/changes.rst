Change Log
==========

**Recorder 2.4.0** Mar 24, 2023

1. Implement inter-process compression using offset pattern detection
2. Clean up code and simplify post-processing APIs

**Recorder 2.3.3** Jan 21, 2022

1. Still require a RECORDER_WITH_NON_MPI hint for non-mpi programs.
2. Add a singal handler to intercept SIGTERM and SIGINT.
3. Allow setting buffer size 4. Fix timestamps in Chrome trace conversion

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

1. Add fcntl() support. Only support commands defined in `POSIX standard <https://pubs.opengroup.org/onlinepubs/009695399/functions/fcntl.html>`__.
2. Add support for MPI_Ibcast()

**Recorder 2.1.6** Nov 05, 2020

1. Generate unique id for communicators
2. Fix bus caused by MPI_COMM_NULL
3. Add support for MPI_File_get_size

**Recorder 2.1.5** Aug 27, 2020

1. Add MPI_File_seek and MPI_File_seek_shared
2. Add documentation on how to install using `Spack <https://spack.io>`__.

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

1. Use `uthash <https://github.com/troydhanson/uthash>`__ library to replace the original hash map implementation
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
3. Write a separate log unit to provide an uniform logging interface. Re-write most of the code to use this new log unit.
4. Ignore files (e.g. /sockets) that are not used by the application itself.
5. Add a built-in hashmap to support mappings from function name and filename to integers.
6. Put all function (that we plan to intercept) signatures in the same header file

