#------------------------------------------------------------------------------
# Include source and build directories
#------------------------------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR}/include)

#------------------------------------------------------------------------------
# Set sources
#------------------------------------------------------------------------------
set(RECORDER_SRCS
        ${CMAKE_SOURCE_DIR}/include/recorder.h
        ${CMAKE_SOURCE_DIR}/include/recorder-sequitur.h
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-hdf5.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-logger.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-cst-cfg.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-mpi.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-init-finalize.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-posix.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-utils.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-function-profiler.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-pattern-recognition.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-sequitur.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-sequitur-symbol.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-sequitur-digram.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-sequitur-logger.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-sequitur-utils.c)


#------------------------------------------------------------------------------
# External dependencies
#------------------------------------------------------------------------------
find_package(HDF5 REQUIRED)
if(HDF5_FOUND)
    include_directories(${HDF5_INCLUDE_DIRS})
    set(RECORDER_EXT_INCLUDE_DEPENDENCIES ${HDF5_INCLUDE_DIRS}
            ${RECORDER_EXT_INCLUDE_DEPENDENCIES})
    set(RECORDER_EXT_LIB_DEPENDENCIES
            ${HDF5_LIBRARIES} ${RECORDER_EXT_LIB_DEPENDENCIES})
else()
    message(STATUS, "HDF5 not found")
endif()

find_package(MPI REQUIRED)
if(MPI_FOUND)
    include_directories(${MPI_CXX_INCLUDE_DIRS})
    set(RECORDER_EXT_INCLUDE_DEPENDENCIES ${MPI_C_INCLUDE_DIRS}
            ${RECORDER_EXT_INCLUDE_DEPENDENCIES})
    set(RECORDER_EXT_LIB_DEPENDENCIES
            ${MPI_C_LIBRARIES} ${RECORDER_EXT_LIB_DEPENDENCIES})
else()
    message(STATUS, "MPI not found")
endif()

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    set(RECORDER_EXT_INCLUDE_DEPENDENCIES ${ZLIB_INCLUDE_DIRS}
            ${RECORDER_EXT_INCLUDE_DEPENDENCIES})
    set(RECORDER_EXT_LIB_DEPENDENCIES
            ${ZLIB_LIBRARIES} ${RECORDER_EXT_LIB_DEPENDENCIES})
else()
    message(STATUS, "ZLIB not found")
endif()


if(RECORDER_ENABLE_CUDA_TRACE)
    find_package(CUDA REQUIRED)
    find_package(CUDAToolkit REQUIRED)
    if(CUDAToolkit_FOUND)
        set(RECORDER_SRCS
            ${CMAKE_CURRENT_SOURCE_DIR}/recorder-cuda-profiler.c
            ${RECORDER_SRCS}
        )

        message("-- " "Found CUDA: TRUE")
        include_directories(${CUDA_INCLUDE_DIRS})
        set(RECORDER_EXT_INCLUDE_DEPENDENCIES
                ${CUDA_INCLUDE_DIRS} ${RECORDER_EXT_INCLUDE_DEPENDENCIES})
        set(RECORDER_EXT_LIB_DEPENDENCIES
                ${CUDA_LIBRARIES} ${RECORDER_EXT_LIB_DEPENDENCIES})
        set(RECORDER_EXT_LIB_DEPENDENCIES
                ${CUDA_cupti_LIBRARY} ${RECORDER_EXT_LIB_DEPENDENCIES})
    else()
        message("-- " "Found CUDA: FALSE")
    endif()
endif()



#------------------------------------------------------------------------------
# Libraries
#------------------------------------------------------------------------------
set(RECORDER_BUILD_INCLUDE_DEPENDENCIES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        )
add_library(recorder ${RECORDER_SRCS})
target_include_directories(recorder
        PUBLIC "$<BUILD_INTERFACE:${RECORDER_BUILD_INCLUDE_DEPENDENCIES}>"
        $<INSTALL_INTERFACE:${RECORDER_INSTALL_INCLUDE_INTERFACE}>
        )

target_link_libraries(recorder
        PUBLIC ${HDF5_LIBRARIES}
        PUBLIC ${MPI_C_LIBRARIES}
        PUBLIC ${RECORDER_EXT_LIB_DEPENDENCIES}
        PUBLIC pthread
        )

target_compile_definitions(recorder
        PUBLIC _LARGEFILE64_SOURCE
        PUBLIC RECORDER_PRELOAD
        PRIVATE $<$<BOOL:${RECORDER_ENABLE_POSIX_TRACE}>:RECORDER_ENABLE_POSIX_TRACE>
        PRIVATE $<$<BOOL:${RECORDER_ENABLE_MPI_TRACE}>:RECORDER_ENABLE_MPI_TRACE>
        PRIVATE $<$<BOOL:${RECORDER_ENABLE_MPIIO_TRACE}>:RECORDER_ENABLE_MPIIO_TRACE>
        PRIVATE $<$<BOOL:${RECORDER_ENABLE_HDF5_TRACE}>:RECORDER_ENABLE_HDF5_TRACE>
        PRIVATE $<$<BOOL:${RECORDER_ENABLE_FCNTL_TRACE}>:RECORDER_ENABLE_FCNTL_TRACE>
        PRIVATE $<$<BOOL:${RECORDER_ENABLE_CUDA_TRACE}>:RECORDER_ENABLE_CUDA_TRACE>
        )

recorder_set_lib_options(recorder "recorder" ${RECORDER_LIBTYPE})

#-----------------------------------------------------------------------------
# Specify project header files to be installed
#-----------------------------------------------------------------------------
set(RECORDER_HEADERS
        ${CMAKE_SOURCE_DIR}/include/recorder.h
        ${CMAKE_SOURCE_DIR}/include/recorder-logger.h
        ${CMAKE_SOURCE_DIR}/include/recorder-utils.h
        ${CMAKE_SOURCE_DIR}/include/uthash.h
)

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
install(
        FILES
        ${RECORDER_HEADERS}
        DESTINATION
        ${RECORDER_INSTALL_INCLUDE_DIR}
        COMPONENT
        headers
)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install
#-----------------------------------------------------------------------------
install(
        TARGETS
        recorder
        EXPORT
        ${RECORDER_EXPORTED_TARGETS}
        LIBRARY DESTINATION ${RECORDER_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${RECORDER_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${RECORDER_INSTALL_BIN_DIR}
)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
install(
        EXPORT
        ${RECORDER_EXPORTED_TARGETS}
        DESTINATION
        ${RECORDER_INSTALL_DATA_DIR}/cmake/recorder
        FILE
        ${RECORDER_EXPORTED_TARGETS}.cmake
)

#-----------------------------------------------------------------------------
# Export all exported targets to the build tree for use by parent project
#-----------------------------------------------------------------------------
if(NOT RECORDER_EXTERNALLY_CONFIGURED)
    EXPORT (
            TARGETS
            ${RECORDER_EXPORTED_LIBS}
            FILE
            ${RECORDER_EXPORTED_TARGETS}.cmake
    )
endif()

#------------------------------------------------------------------------------
# Set variables for parent scope
#------------------------------------------------------------------------------
# Used by config.cmake.build.in and Testing
set(RECORDER_INCLUDES_BUILD_TIME
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${RECORDER_EXT_INCLUDE_DEPENDENCIES}
        PARENT_SCOPE
        )

# Used by config.cmake.install.in
set(RECORDER_INCLUDES_INSTALL_TIME
        ${RECORDER_INSTALL_INCLUDE_DIR}
        ${RECORDER_EXT_INCLUDE_DEPENDENCIES}
        PARENT_SCOPE
        )
