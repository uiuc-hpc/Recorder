Overview
========

We believe that multi-level I/O tracing and trace data analysis tool can
help end users understand the behavior of their application and I/O
subsystem, and can provide insights into the source of I/O performance
bottlenecks.

Recorder is a multi-level I/O tracing framework that can capture I/O
function calls at multiple levels of the I/O stack, including HDF5,
MPI-IO, and POSIX I/O. Recorder requires no modification or
recompilation of the application and users can control what levels are
traced.


Publications
------------

`Wang, Chen, Jinghan Sun, Marc Snir, Kathryn Mohror, and Elsa
Gonsiorowski. “Recorder 2.0: Efficient Parallel I/O Tracing and
Analysis.” In IEEE International Workshop on High-Performance Storage
(HPS), 2020. <https://doi.org/10.1109/IPDPSW50202.2020.00176>`__

`Wang, Chen, Kathryn Mohror, and Marc Snir. “File System Semantics
Requirements of HPC Applications.” Proceedings of the 30th International
Symposium on High-Performance Parallel and Distributed Computing (HPDC).
2021. <https://dl.acm.org/doi/abs/10.1145/3431379.3460637>`__

`Luu, Huong, Babak Behzad, Ruth Aydt, and Marianne Winslett. “A
multi-level approach for understanding I/O activity in HPC
applications.” In 2013 IEEE International Conference on Cluster
Computing (CLUSTER),
2013. <https://doi.org/10.1109/CLUSTER.2013.6702690>`__

Dataset
-------

`Traces from 17 HPC applications <https://doi.org/10.6075/J0Z899X4>`__

The traces were collected using an old version of Recorder. The current
version uses a different trace format. To read those traces please use
Recorder 2.2.1 from the
`release <https://github.com/uiuc-hpc/Recorder/releases/tag/v2.2.1>`__
page.
