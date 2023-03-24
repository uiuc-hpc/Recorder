Usage
=====


Assume ``$RECORDER_ROOT`` is the location where you installed Recorder.

Generate traces
------------------

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

Human-readable traces
------------------------

Recorder uses its own binary tracing format to compress and store
traces.

We provide a tool (recorder2text) that can convert the recorder format
traces to plain text format.

.. code:: bash

   $RECORDER_ROOT/bin/recorder2text /path/to/your_trace_folder/

This will generate text fomart traces under
``/path/to/your_trace_folder/_text``.
