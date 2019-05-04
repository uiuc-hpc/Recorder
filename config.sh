#!/bin/bash

usage() {
  echo "USAGE: $0 <hdf5_dir> <mpi_dir> <layers_to_be_disabled>"
  echo "EXAMPLE: $0 /opt/hdf5-1.8.9 /opt/mpich2-1.4.1p1/ -DDISABLE_MPIO_TRACE -DDISABLE_POSIX_TRACE"
}

if [ "$#" -eq 0 ]
then
    usage
else
  echo HDF5_DIR=$1 > config.inc
  echo MPI_DIR=$2 >> config.inc
  echo DISABLED_LAYERS=${*:3} >> config.inc
fi
