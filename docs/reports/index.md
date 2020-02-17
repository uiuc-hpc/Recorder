Reports
-----------

### FLASH 4.4

#### 1. [Sod 2D without collective I/O](./sod_2d_nofbs.html)

Setup command: `Sod -auto -2d +ug +parallelio -nxb=64 -nyb=64`
Problem size: 512 x 512
MPI: 64 MPI Processes - 8 nodes and 8 MPI ranks per node
Remark: This setup makes the problem scales with the number of processes used
        The problem size: nxb * iProcs, nyb * jProcs
        This configuration uses collective I/O.

#### 2. [Sod 2D with collective I/O](./sod_2d_ug.html)

Setup command: `Sod -auto -2d +ug +nofbs +parallelio`
Problem size: 512 x 512
MPI: 64 MPI Processes - 8 nodes and 8 MPI ranks per node
Remark: With +nofbs, the problem size is spcificied in flash.par, and FLASH can not use collective I/O.


### ENZO 

#### 1. [CollapseTestNonCosmological](./CollapseTestNonCosmological.html)
Problem Size: 64 x 64 x 64
MPI: 64 MPI Processes - 8 nodes and 8 MPI ranks per node
Configuration file:
