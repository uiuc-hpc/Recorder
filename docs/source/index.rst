|build|

Overview
========

Recorder is a multi-level I/O tracing framework that can capture I/O function calls at multiple levels of the I/O stack, including HDF5, PnetCDF, NetCDF, MPI-IO, and POSIX I/O. Recorder requires no modification or recompilation of the application and users can control what levels are traced.

Publications
------------

Recorder has undergone significant changes since the last
"Recorder 2.0" paper. We have incorporated a new pattern-based
compression algorithm, along with many new features.
We have submitted a journal paper describing all these
changes in detail. The draft version is available on `arXiv <https://arxiv.org/abs/2501.04654>`__.

`Wang, Chen, Jinghan Sun, Marc Snir, Kathryn Mohror, and Elsa
Gonsiorowski. “Recorder 2.0: Efficient Parallel I/O Tracing and
Analysis.” In IEEE International Workshop on High-Performance Storage
(HPS), 2020. <https://doi.org/10.1109/IPDPSW50202.2020.00176>`__


I/O Patterns and consistency requirements
-----------------------------------------

`Traces from 17 HPC applications <https://doi.org/10.6075/J0Z899X4>`__

The traces were collected using an old version of Recorder. The current
version uses a different trace format. To read those traces please use
Recorder 2.2.1 from the
`release <https://github.com/uiuc-hpc/Recorder/releases/tag/v2.2.1>`__
page.

The following paper presents a comprehensive study on the I/O patterns
of these applications. Additionally, it investigates the consistency
requirements of these applications, confirming experimentally that
POSIX consistency is rarely required by HPC applications.

`Wang, Chen, Kathryn Mohror, and Marc Snir. “File System Semantics
Requirements of HPC Applications.” Proceedings of the 30th International
Symposium on High-Performance Parallel and Distributed Computing (HPDC).
2021. <https://dl.acm.org/doi/abs/10.1145/3431379.3460637>`__


.. toctree::
   :hidden
   quickstart
   build
   usage
   features
   postprocessing
   changes


.. |build| image:: https://github.com/uiuc-hpc/Recorder/actions/workflows/cmake.yml/badge.svg
   :target: https://github.com/uiuc-hpc/Recorder/actions/workflows/cmake.yml
