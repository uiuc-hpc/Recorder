

lib_LTLIBRARIES = librecorder.la


disabled_layers = $(DISABLE_POSIX) $(DISABLE_MPI) $(DISABLE_HDF5) $(DISABLE_FCNTL)

librecorder_la_SOURCES = recorder-hdf5.c  recorder-logger.c  recorder-mpi.c  recorder-mpi-init-finalize.c  recorder-posix.c  recorder-utils.c
librecorder_la_CFLAGS = -I$(top_srcdir)/include -D_LARGEFILE64_SOURCE -DRECORDER_PRELOAD $(disabled_layers)
librecorder_la_LDFLAGS = -lpthread -lhdf5 -version-info 0:0:0

