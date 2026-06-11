Usage
=====


Assume ``$RECORDER_INSTALL_PATH`` is the location where you installed Recorder.

Generate traces
------------------

.. code:: bash

   # For MPI programs
   mpirun -np $nprocs -env LD_PRELOAD $RECORDER_INSTALL_PATH/lib/librecorder.so ./your_app

   # With srun
   srun -n $nprocs --export=ALL,LD_PRELOAD=$RECORDER_INSTALL_PATH/lib/librecorder.so ./your_app

   # With FLUX
   flux run $nprocs --env LD_PRELOAD=RECORDER_INSTALL_PATH/lib/librecorder.so ./your_app

   # For non-MPI programs or programs that may spwan non-mpi children programs
   RECORDER_WITH_NON_MPI=1 LD_PRELOAD=$RECORDER_INSTALL_PATH/lib/librecorder.so ./your_app

The trace files will be written to the current directory under a folder
named ``hostname-username-appname-pid-starttime``.

*Note: In some systems (e.g., Quartz at LLNL), Darshan is deployed
system-widely. Recorder does not work with Darshan. Please make sure
darhsn is disabled and your application is not linked with the darshan
library (use ldd to check).*

Configure tracing layers
------------------------

Currently, Recorder is capable of tracing POSIX, MPI, MPI-IO, HDF5, NetCDF, PnetCDF and
DAOS calls. DAOS tracing covers both the DAOS File System (DFS) API and the native DAOS
API (container, object, array and key-value), and is only available when Recorder was
built with DAOS (see the build instructions).
By default, all supported I/O layers are enabled.
At runtime (generally before running your application), you can set
the following environment variables to dynamically enable/disable
the tracing of certain layers.

1 = enable; 0 = disable.

* export RECORDER_POSIX_TRACING=[1|0] (default: 1)

* export RECORDER_MPIIO_TRACING=[1|0] (default: 1)

* export RECORDER_HDF5_TRACING=[1|0] (default: 1)

* export RECORDER_NETCDF_TRACING=[1|0] (default: 1)

* export RECORDER_PNETCDF_TRACING=[1|0] (default: 1)

* export RECORDER_DAOS_TRACING=[1|0] (default: 1)

* export RECORDER_MPI_TRACING=[1|0] (default: 0)


Human-readable traces
------------------------

Recorder uses its own binary tracing format to compress and store
traces.

We provide a tool (recorder2text) that can convert the recorder format
traces to plain text format.

.. code:: bash

   $RECORDER_INSTALL_PATH/bin/recorder2text /path/to/your_trace_folder/

This will generate text fomart traces under
``/path/to/your_trace_folder/_text``.
