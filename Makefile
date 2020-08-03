ifneq ("$(wildcard config.inc)","")
	include config.inc
else
	$(error Please run config.sh)
endif

srcdir = .
prefix = .
libdir = ${prefix}/librecorder/lib

CC = ${MPI_DIR}/bin/mpicc
ifeq ("$(wildcard $(CC))","")
	CC = mpicc
endif

LD = @LD@

INCL_DEPS = include/recorder.h include/recorder-log-format.h include/uthash.h

CFLAGS_SHARED = -shared -fPIC -I. -I$(srcdir)/include -I$(srcdir)/../\
    -I${MPI_DIR}/include -I/${HDF5_DIR}/include/ \
    -D_LARGEFILE64_SOURCE -DRECORDER_PRELOAD

LIBS += -lz @LIBBZ2@
LDFLAGS += -L${MPI_DIR}/lib -L${HDF5_DIR}/lib -lhdf5 -ldl

CFLAGS += $(CFLAGS_SHARED) ${DISABLED_LAYERS}

all: tools/C/reader.so lib/librecorder.so

lib:
	@mkdir -p $@

%.po: %.c $(INCL_DEPS) | lib
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

lib/librecorder.so: lib/recorder-mpi.po lib/recorder-mpi-init-finalize.po lib/recorder-hdf5.po lib/recorder-posix.po lib/recorder-logger.po lib/recorder-utils.po
	$(CC) $(CFLAGS) -o $@ $^ -lpthread -lrt -lz $(LDFLAGS)

tools/C/reader.so:
	$(CC) -fPIC -shared -ldl tools/C/reader.c -o $@

install:: all
	install -d $(libdir)
	install -m 755 lib/librecorder.so $(libdir)

clean::
	rm -f *.o *.a lib/*.o lib/*.po lib/*.a lib/*.so

distclean:: clean
