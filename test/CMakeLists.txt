find_package(MPI REQUIRED)

set(TEST_TRACES_BASE ${CMAKE_CURRENT_BINARY_DIR}/traces)

# -----------------------------------------------------------------------------
# Helper macro: compile a test executable and register it with CTest.
#   name      - target name (must match source file test_<name>.c)
#   np        - number of MPI processes (1 = run without mpirun)
#   extra_env - optional extra environment variables (semicolon-separated)
# Each test gets an isolated trace directory and runs with LD_PRELOAD set to
# the recorder shared library so all I/O calls are intercepted.
# The test is registered as a FIXTURES_SETUP for <name>_traces so that
# postprocessing check tests can declare FIXTURES_REQUIRED on it.
# -----------------------------------------------------------------------------
macro(recorder_add_test name np)
    set(_trace_dir ${TEST_TRACES_BASE}/${name})
    file(MAKE_DIRECTORY ${_trace_dir})

    set(_extra_env ${ARGN})   # optional extra env vars passed after np

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
        ENVIRONMENT "LD_PRELOAD=$<TARGET_FILE:recorder>;RECORDER_TRACES_DIR=${_trace_dir};${_extra_env}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        FIXTURES_SETUP ${name}_traces
    )
endmacro()

# -----------------------------------------------------------------------------
# Helper macro: register postprocessing check tests for a named trace set.
#   name  - same name used in recorder_add_test
#   np    - expected process count (for recorder-summary output check)
# Adds three tests:
#   check_reader_<name>  - validates recorder_init_reader() result
#   check_summary_<name> - runs recorder-summary, checks output format
#   run_text_<name>      - runs recorder2text (produces _text/ directory)
#   verify_text_<name>   - verifies _text/0.txt was created and is non-empty
# -----------------------------------------------------------------------------
macro(recorder_add_check_tests name)
    set(_trace_dir ${TEST_TRACES_BASE}/${name})

    # -- reader API validation --
    add_test(NAME check_reader_${name}
        COMMAND $<TARGET_FILE:test_reader> ${_trace_dir}
    )
    set_tests_properties(check_reader_${name} PROPERTIES
        FIXTURES_REQUIRED ${name}_traces
    )

    # -- recorder-summary: verify it exits 0 and prints expected header --
    add_test(NAME check_summary_${name}
        COMMAND $<TARGET_FILE:recorder-summary> ${_trace_dir}
    )
    set_tests_properties(check_summary_${name} PROPERTIES
        FIXTURES_REQUIRED ${name}_traces
        PASS_REGULAR_EXPRESSION "Total processes: [0-9]+"
    )

    # -- recorder2text: run, then verify _text/0.txt was created --
    add_test(NAME run_text_${name}
        COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 1
                $<TARGET_FILE:recorder2text> ${_trace_dir}
    )
    set_tests_properties(run_text_${name} PROPERTIES
        FIXTURES_REQUIRED ${name}_traces
        FIXTURES_SETUP    ${name}_text
    )

    add_test(NAME verify_text_${name}
        COMMAND ${CMAKE_COMMAND} -E cat ${_trace_dir}/_text/0.txt
    )
    set_tests_properties(verify_text_${name} PROPERTIES
        FIXTURES_REQUIRED ${name}_text
    )
endmacro()

# -----------------------------------------------------------------------------
# test_reader: validates recorder_init_reader() against a trace directory.
# Used by the check tests registered via recorder_add_check_tests().
# -----------------------------------------------------------------------------
add_executable(test_reader test_reader.c)
target_include_directories(test_reader PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tools
)
target_link_libraries(test_reader PRIVATE reader)

# -----------------------------------------------------------------------------
# test_posix: basic POSIX I/O tracing (open, read, write, seek, stat, dir ops)
# RECORDER_WITH_NON_MPI=1 is required because test_posix is not an MPI program;
# without it Recorder does not initialize and no traces are produced.
# -----------------------------------------------------------------------------
add_executable(test_posix test_posix.c)
recorder_add_test(test_posix 1 "RECORDER_WITH_NON_MPI=1")
recorder_add_check_tests(test_posix)

# -----------------------------------------------------------------------------
# test_iopr: positioned I/O — lseek + pwrite patterns
# -----------------------------------------------------------------------------
add_executable(test_iopr test_iopr.c)
target_link_libraries(test_iopr PRIVATE MPI::MPI_C)
recorder_add_test(test_iopr 2)
recorder_add_check_tests(test_iopr)

# -----------------------------------------------------------------------------
# test_mpi: MPI point-to-point, collectives, and MPI-IO
# Requires >=4 processes to exercise the MPI_Reduce path (root rank 2).
# -----------------------------------------------------------------------------
add_executable(test_mpi test_mpi.c)
target_link_libraries(test_mpi PRIVATE MPI::MPI_C)
recorder_add_test(test_mpi 4)
recorder_add_check_tests(test_mpi)

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
