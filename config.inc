HDF5_DIR=/usr/tce/packages/hdf5/hdf5-parallel-1.8.18-intel-18.0.1-openmpi-2.0.0/
MPI_DIR=/usr/tce/packages/openmpi/openmpi-2.0.0-intel-18.0.1/

# Only For VASP
#HDF5_DIR=/usr/tce/package/hdf5-parallel-1.8.18-intel-18.0.1-mvapich2-2.2/
#MPI_DIR=/usr/tce/packages/mvapich2/mvapich2-2.2-intel-18.0.1/

# Only for LBANN
# Also need to set LD_LIBRARY_PATH when running the LBANN
#MPI_DIR=/usr/tce/packages/mvapich2/mvapich2-2.3-gcc-7.3.0/
#HDF5_DIR=/g/g90/wang116/softwares/lbann/build/gnu.Release.quartz.llnl.gov/install


#DISABLED_LAYERS=-DDISABLE_HDF5_TRACE -DDISABLE_MPIO_TRACE
#DISABLED_LAYERS=-DDISABLE_HDF5_TRACE -DDISABLE_POSIX_TRACE
DISABLED_LAYERS=-DDISABLE_HDF5_TRACE
