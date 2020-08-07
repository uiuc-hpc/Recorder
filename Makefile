ifneq ("$(wildcard config.inc)","")
	include config.inc
else
	$(error Please run config.sh)
endif

srcdir = .
prefix = .
libdir = ${prefix}/librecorder/lib
bindir = ${prefix}/librecorder/bin

CC = ${MPI_DIR}/bin/mpicc
CXX = ${MPI_DIR}/bin/mpicxx

ifeq ("$(wildcard $(CC))","")
	CC = mpicc
	CXX = mpicxx
endif


CFLAGS_SHARED = -shared -fPIC -I$(srcdir)/include \
    -I${MPI_DIR}/include -I/${HDF5_DIR}/include/ \
    -D_LARGEFILE64_SOURCE -DRECORDER_PRELOAD

CFLAGS += $(CFLAGS_SHARED) ${DISABLED_LAYERS}

LDFLAGS += -L${MPI_DIR}/lib -L${HDF5_DIR}/lib -lhdf5 -ldl


all: lib/librecorder.so tools/reader.so tools/recorder2text.out tools/overlap_conflict.out

%.po: %.c | lib
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

lib/librecorder.so: lib/recorder-mpi.po lib/recorder-mpi-init-finalize.po lib/recorder-hdf5.po lib/recorder-posix.po lib/recorder-logger.po lib/recorder-utils.po
	$(CC) $(CFLAGS) -o $@ $^ -lpthread -lrt -lz $(LDFLAGS)

tools/reader.so: tools/reader.c
	$(CC) -fPIC -shared -ldl -I$(srcdir)/include $^ -o $@

tools/recorder2text.out: tools/recorder2text.c tools/reader.c
	$(CC) -I$(srcdir)/include $^ -o $@

tools/overlap_conflict.out: tools/overlap_conflict.c tools/reader.c tools/build_offset_intervals.cpp
	$(CXX) -I$(srcdir)/include $^ -o $@


install:: all
	install -d $(libdir) $(bindir)
	install -m 755 lib/librecorder.so $(libdir)
	install -m 755 tools/reader.so $(libdir)
	install -m 755 tools/recorder2text.out $(bindir)
	install -m 755 tools/overlap_conflict.out $(bindir)

clean::
	rm -f *.o *.a lib/*.o lib/*.po lib/*.a lib/*.so tools/*.so tools/*.out

distclean:: clean
