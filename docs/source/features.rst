Other Features 
===========================================

Inclusion/Exclusion prefix list
-------------------------------

Many POSIX calls intercepted are made by system libraries, job
schedulers, etc. Their I/O accesses are less interesting as they
operates on file locations such as ``/dev, /sys, /usr/lib, etc``. To
ignore those calls, you can specifiy a file that contains prefixes that
you want to exclude:

.. code:: bash

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

Similarly, you can set ``RECORDER_INCLUSION_FILE`` to specify the
inclusion prefixes, so only the POSIX calls that match those prefixes
will be recorded.

Note that this feature only applies to POSIX calls. MPI and HDF5 calls
are always recorded when enabled.

Storing pointers
----------------

Recorder by default does not log the pointers (memory addresses) as they
provide little information yet cost a lot of space to store. However,
you can change this behaviour by setting the enviroment variable
``RECORDER_LOG_POINTER`` to 1.

Storing thread ids
------------------

Use ``RECORDER_LOG_TID``\ (0 or 1) to control whether to store thread
id. Default is 0.

Storing call levels
-------------------

Use ``RECORDER_LOG_LEVEL`` (0 or 1) to control whether to store call
levels. Default is 1.

Traces location
---------------

By default Recorder will output the traces to the current working
directory. You can use the enviroment variable ``RECORDER_TRACES_DIR``
to specifiy the path where you want the traces stored. Make sure that
every process has the persmission to write to that directory.

Timestamp buffer size
-----------

Timestamps are buffered internally to avoid frequent disk I/O. Use
``RECORDER_BUFFER_SIZE`` (in MB) to set the size of this buffer. The
default value is 1MB.
