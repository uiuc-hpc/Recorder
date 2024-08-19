#------------------------------------------------------------------------------
# Include source and build directories
#------------------------------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/deps/GOTCHA/include)

# set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#------------------------------------------------------------------------------
# Set sources
#------------------------------------------------------------------------------
set(RECORDER_SRCS
        ${CMAKE_SOURCE_DIR}/include/recorder.h
        ${CMAKE_SOURCE_DIR}/include/recorder-sequitur.h
        ${CMAKE_SOURCE_DIR}/include/recorder-analysis.h
        ${CMAKE_SOURCE_DIR}/include/recorder-optimization.h
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-hdf5.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-cst-cfg.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-mpi.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-init-finalize.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-posix.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-utils.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-logger.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-gotcha.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-function-profiler.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-pattern-recognition.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-timestamps.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-sequitur.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-sequitur-symbol.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-sequitur-digram.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-sequitur-logger.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-sequitur-utils.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-analysis.c
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-optimization.c)


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

# PnetCDF is not a CMAKE project, we use find_path() to locate it
# this requires users to set -DRECORDER_WITH_PNETCDF=/xx/xx/
find_path(PNETCDF_INCLUDE_DIRS pnetcdf.h ${RECORDER_WITH_PNETCDF}/include)
find_path(PNETCDF_LIBRARY_FOUND libpnetcdf.so ${RECORDER_WITH_PNETCDF}/lib)
if(PNETCDF_INCLUDE_DIRS AND PNETCDF_LIBRARY_FOUND)
    message("-- " "Found PnetCDF:" ${RECORDER_WITH_PNETCDF})
    set(RECORDER_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-pnetcdf.c
        ${RECORDER_SRCS}
    )
    include_directories(${PNETCDF_INCLUDE_DIRS})
    set(PNETCDF_LIBRARIES ${RECORDER_WITH_PNETCDF}/lib/libpnetcdf.so)
    set(RECORDER_EXT_INCLUDE_DEPENDENCIES ${PNETCDF_INCLUDE_DIRS}
            ${RECORDER_EXT_INCLUDE_DEPENDENCIES})
    set(RECORDER_EXT_LIB_DEPENDENCIES
            ${PNETCDF_LIBRARIES} ${RECORDER_EXT_LIB_DEPENDENCIES})
else()
    message("-- " "PnetCDF not found. Will build Recorder without it.")
endif()

# Look for NetCDF
# -DRECORDER_WITH_NETCDF=/xx/xx/
find_path(NETCDF_INCLUDE_DIRS netcdf.h ${RECORDER_WITH_NETCDF}/include)
find_path(NETCDF_LIBRARY_FOUND libnetcdf.so ${RECORDER_WITH_NETCDF}/lib)
if(NETCDF_INCLUDE_DIRS AND NETCDF_LIBRARY_FOUND)
    message("-- " "Found NetCDF:" ${RECORDER_WITH_NETCDF})
    set(RECORDER_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/recorder-netcdf.c
        ${RECORDER_SRCS}
    )
    include_directories(${NETCDF_INCLUDE_DIRS})
    set(NETCDF_LIBRARIES ${RECORDER_WITH_NETCDF}/lib/libnetcdf.so)
    set(RECORDER_EXT_INCLUDE_DEPENDENCIES ${NETCDF_INCLUDE_DIRS}
            ${RECORDER_EXT_INCLUDE_DEPENDENCIES})
    set(RECORDER_EXT_LIB_DEPENDENCIES
            ${NETCDF_LIBRARIES} ${RECORDER_EXT_LIB_DEPENDENCIES})
else()
    message("-- " "NetCDF not found. Will build Recorder without it.")
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


INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(__xstat    HAVE___XSTAT)
CHECK_FUNCTION_EXISTS(__xstat64  HAVE___XSTAT64)
CHECK_FUNCTION_EXISTS(__lxstat   HAVE___LXSTAT)
CHECK_FUNCTION_EXISTS(__lxstat64 HAVE___LXSTAT64)
CHECK_FUNCTION_EXISTS(__fxstat   HAVE___FXSTAT)
CHECK_FUNCTION_EXISTS(__fxstat64 HAVE___FXSTAT64)


#------------------------------------------------------------------------------
# Libraries
#------------------------------------------------------------------------------
set(RECORDER_BUILD_INCLUDE_DEPENDENCIES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        )
add_library(recorder ${RECORDER_SRCS})
#set_property(TARGET recorder PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(recorder
        PUBLIC "$<BUILD_INTERFACE:${RECORDER_BUILD_INCLUDE_DEPENDENCIES}>"
        $<INSTALL_INTERFACE:${RECORDER_INSTALL_INCLUDE_INTERFACE}>
        )

target_link_libraries(recorder
        PUBLIC gotcha   # this must match add_library(target) in GOTCHA project,
        PUBLIC ${RECORDER_EXT_LIB_DEPENDENCIES}
        PUBLIC pthread
        )

target_compile_definitions(recorder
        PUBLIC _LARGEFILE64_SOURCE
        PUBLIC RECORDER_PRELOAD
        PRIVATE $<$<BOOL:${HAVE___XSTAT}>:HAVE___XSTAT>
        PRIVATE $<$<BOOL:${HAVE___XSTAT64}>:HAVE___XSTAT64>
        PRIVATE $<$<BOOL:${HAVE___LXSTAT}>:HAVE___LXSTAT>
        PRIVATE $<$<BOOL:${HAVE___LXSTAT64}>:HAVE___LXSTAT64>
        PRIVATE $<$<BOOL:${HAVE___FXSTAT}>:HAVE___FXSTAT>
        PRIVATE $<$<BOOL:${HAVE___FXSTAT64}>:HAVE___FXSTAT64>
        PRIVATE $<$<AND:$<BOOL:${PNETCDF_INCLUDE_DIRS}>,$<BOOL:${PNETCDF_LIBRARY_FOUND}>>:RECORDER_WITH_PNETCDF>
        PRIVATE $<$<AND:$<BOOL:${NETCDF_INCLUDE_DIRS}>,$<BOOL:${NETCDF_LIBRARY_FOUND}>>:RECORDER_WITH_NETCDF>
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
