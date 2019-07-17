HDF5_DIR=/usr/tce/packages/hdf5/hdf5-parallel-1.8.18-intel-18.0.1-openmpi-2.0.0/
MPI_DIR=/usr/tce/packages/openmpi/openmpi-2.0.0-intel-18.0.1/

# Only For VASP
#HDF5_DIR=/usr/tce/package/hdf5-parallel-1.8.18-intel-18.0.1-mvapich2-2.2/
#MPI_DIR=/usr/tce/packages/mvapich2/mvapich2-2.2-intel-18.0.1/

# Only for LBANN
# Also need to set LD_LIBRARY_PATH when running the LBANN
# export LD_LIBRARY_PATH=/p/lscratchh/wang116/applications/lbann/install/lib:/p/lscratchh/wang116/applications/lbann/install/lib64:$LD_LIBRARY_PATH
#MPI_DIR=/usr/tce/packages/mvapich2/mvapich2-2.3-gcc-7.3.0/
#HDF5_DIR=/p/lscratchh/wang116/applications/lbann/install/

# Only for Chombo
#MPI_DIR=/usr/tce/packages/openmpi/openmpi-2.0.0-gcc-4.9.3/
#HDF5_DIR=/usr/tce/packages/hdf5/hdf5-parallel-1.8.18-gcc-4.9.3-openmpi-2.0.0

# On personal machine at UIUC
#MPI_DIR=/opt/mpich2/
#HDF5_DIR=/home/wangchen/softwares/hdf5-1.8.16/hdf5

#DISABLED_LAYERS=-DDISABLE_HDF5_TRACE -DDISABLE_MPIO_TRACE
#DISABLED_LAYERS=-DDISABLE_HDF5_TRACE -DDISABLE_POSIX_TRACE
#DISABLED_LAYERS=-DDISABLE_HDF5_TRACE
