

lib_LTLIBRARIES = librecorder.la
librecorder_la_SOURCES = recorder-hdf5.c  recorder-logger.c  recorder-mpi.c  recorder-mpi-init-finalize.c  recorder-posix.c  recorder-utils.c
librecorder_la_CPPFLAGS = -I$(top_srcdir)/include -D_LARGEFILE64_SOURCE -DRECORDER_PRELOAD
librecorder_la_LDFLAGS = -lpthread -lhdf5 -version-info 0:0:0
