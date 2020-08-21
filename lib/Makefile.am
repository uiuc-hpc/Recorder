

lib_LTLIBRARIES = librecorder.la
librecorder_la_SOURCES = recorder-hdf5.c  recorder-logger.c  recorder-mpi.c  recorder-mpi-init-finalize.c  recorder-posix.c  recorder-utils.c
librecorder_la_CPPFLAGS = -I$(top_srcdir)/include
librecorder_la_LDFLAGS = -lhdf5 -lz -ldl -version-info 2:1:2
