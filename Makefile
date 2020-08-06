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


all: lib/librecorder.so tools/C/reader.so tools/C/recorder2text.out tools/C/overlap_conflict.out

%.po: %.c | lib
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

lib/librecorder.so: lib/recorder-mpi.po lib/recorder-mpi-init-finalize.po lib/recorder-hdf5.po lib/recorder-posix.po lib/recorder-logger.po lib/recorder-utils.po
	$(CC) $(CFLAGS) -o $@ $^ -lpthread -lrt -lz $(LDFLAGS)

tools/C/reader.so: tools/C/reader.c
	$(CC) -fPIC -shared -ldl $^ -o $@

tools/C/recorder2text.out: tools/C/recorder2text.c tools/C/reader.c
	$(CC) $^ -o $@

tools/C/overlap_conflict.out: tools/C/overlap_conflict.c tools/C/reader.c tools/C/build_offset_intervals.cpp
	$(CXX) $^ -o $@


install:: all
	install -d $(libdir) $(bindir)
	install -m 755 lib/librecorder.so $(libdir)
	install -m 755 tools/C/reader.so $(libdir)
	install -m 755 tools/C/recorder2text.out $(bindir)
	install -m 755 tools/C/overlap_conflict.out $(bindir)

clean::
	rm -f *.o *.a lib/*.o lib/*.po lib/*.a lib/*.so tools/C/*.so tools/C/recorder2text.out

distclean:: clean
