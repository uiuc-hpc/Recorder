[![build](https://github.com/uiuc-hpc/Recorder/actions/workflows/cmake.yml/badge.svg)](https://github.com/uiuc-hpc/Recorder/actions/workflows/cmake.yml)

# Recorder

**Comprehensive Parallel I/O Tracing and Analysis**

Recorder is a multi-level I/O tracing framework that can capture I/O function
calls at multiple levels of the I/O stack, including HDF5, PnetCDF, NetCDF, MPI-IO,
and POSIX I/O. Recorder requires no modification or recompilation of the application and
users can control what levels are traced.


### Quickstart

Build:

```bash
git clone https://github.com/uiuc-hpc/Recorder.git
cd Recorder
export RECORDER_INSTALL_PATH=`pwd`/install
git submodule update --init --recursive
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$RECORDER_INSTALL_PATH
make && make install
```

Run:
```bash
mpirun -np N -env LD_PRELOAD $RECORDER_INSTALL_PATH/lib/librecorder.so ./your-app

# On HPC systems, you may need to use srun or
# other job schedulers to replace mpirun, e.g.,
srun -n4 -N1 --overlap --export=ALL,LD_PRELOAD=$RECORDER_INSTALL_PATH/lib/librecorder.so ./your-app
flux run -n 4 --env LD_PRELOAD=$RECORDER_INSTALL_PATH/ilb/librecorder.so ./your-app
```


### Documentation

Recorder documentation is at [https://recorder.readthedocs.io/](https://recorder.readthedocs.io/).


### Citation

*Wang, Chen, Jinghan Sun, Marc Snir, Kathryn Mohror, and Elsa Gonsiorowski. “Recorder 2.0: Efficient Parallel I/O Tracing and Analysis.” In IEEE International Workshop on High-Performance Storage (HPS), 2020.*
[https://ieeexplore.ieee.org/abstract/document/9150354](https://ieeexplore.ieee.org/abstract/document/9150354)

Recorder has undergone substantial updates since the publication of the 'Recorder 2.0' paper. These include the integration of a new pattern-based compression algorithm, expanded support for the complete set of HDF5, NetCDF, and PnetCDF calls, as well as numerous additional features. For detailed information, please refer to and cite the latest version here: [https://arxiv.org/abs/2501.04654](https://arxiv.org/abs/2501.04654).

