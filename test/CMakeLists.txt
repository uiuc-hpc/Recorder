find_package(MPI REQUIRED)

set(TEST_TRACES_BASE ${CMAKE_CURRENT_BINARY_DIR}/traces)

# -----------------------------------------------------------------------------
# Helper macro: compile a test executable and register it with CTest.
#   name  - target name (must match source file test_<name>.c)
#   np    - number of MPI processes (1 = run without mpirun)
# Each test gets an isolated trace directory and runs with LD_PRELOAD set to
# the recorder shared library so all I/O calls are intercepted.
# -----------------------------------------------------------------------------
macro(recorder_add_test name np)
    set(_trace_dir ${TEST_TRACES_BASE}/${name})
    file(MAKE_DIRECTORY ${_trace_dir})

    if(${np} GREATER 1)
        add_test(
            NAME ${name}
            COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${np}
                    ${MPIEXEC_PREFLAGS} $<TARGET_FILE:${name}> ${MPIEXEC_POSTFLAGS}
        )
    else()
        add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)
    endif()

    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "LD_PRELOAD=$<TARGET_FILE:recorder>;RECORDER_TRACES_DIR=${_trace_dir}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endmacro()

# -----------------------------------------------------------------------------
# test_posix: basic POSIX I/O tracing (open, read, write, seek, stat, dir ops)
# -----------------------------------------------------------------------------
add_executable(test_posix test_posix.c)
recorder_add_test(test_posix 1)

# -----------------------------------------------------------------------------
# test_iopr: positioned I/O — lseek + pwrite patterns
# -----------------------------------------------------------------------------
add_executable(test_iopr test_iopr.c)
target_link_libraries(test_iopr PRIVATE MPI::MPI_C)
recorder_add_test(test_iopr 2)

# -----------------------------------------------------------------------------
# test_mpi: MPI point-to-point, collectives, and MPI-IO
# Requires >=4 processes to exercise the MPI_Reduce path (root rank 2).
# -----------------------------------------------------------------------------
add_executable(test_mpi test_mpi.c)
target_link_libraries(test_mpi PRIVATE MPI::MPI_C)
recorder_add_test(test_mpi 4)

# -----------------------------------------------------------------------------
# test_signal: signal-handling test — compiled but not registered as a CTest
# target. It sleeps 20 s waiting for an external kill signal and triggers a
# segfault in ts_merge_files during MPI_Finalize, making it unsuitable for
# automated runs. Run it manually to test signal-handling behaviour.
# -----------------------------------------------------------------------------
add_executable(test_signal test_signal.c)
target_link_libraries(test_signal PRIVATE MPI::MPI_C)

# -----------------------------------------------------------------------------
# test_hybrid: fork/exec test — compiled but not registered as a CTest target
# because it relies on a hardcoded mpirun path and the sibling test_mpi binary
# being in the current working directory, which is fragile in CI environments.
# -----------------------------------------------------------------------------
add_executable(test_hybrid test_hybrid.c)
target_link_libraries(test_hybrid PRIVATE MPI::MPI_C)

# -----------------------------------------------------------------------------
# HDF5 tests — only built and registered when HDF5 was detected at configure time
# -----------------------------------------------------------------------------
if(HDF5_FOUND)
    # test_hdf5: sequential HDF5 file/dataset/dataspace operations
    add_executable(test_hdf5 test_hdf5.c)
    target_include_directories(test_hdf5 PRIVATE ${HDF5_INCLUDE_DIRS})
    target_link_libraries(test_hdf5 PRIVATE ${HDF5_LIBRARIES} MPI::MPI_C)
    recorder_add_test(test_hdf5 1)

    # test_phdf5: parallel HDF5 — collective write and attribute operations
    add_executable(test_phdf5 test_phdf5.c)
    target_include_directories(test_phdf5 PRIVATE ${HDF5_INCLUDE_DIRS})
    target_link_libraries(test_phdf5 PRIVATE ${HDF5_LIBRARIES} MPI::MPI_C)
    recorder_add_test(test_phdf5 2)
endif()
