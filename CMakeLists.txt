cmake_minimum_required (VERSION 3.10)
project(RECORDER)
include_directories(${PROJECT_BINARY_DIR})
#------------------------------------------------------------------------------
# Version information
#------------------------------------------------------------------------------
set(RECORDER_VERSION_MAJOR "2")
set(RECORDER_VERSION_MINOR "3")
set(RECORDER_VERSION_PATCH "2")
set(RECORDER_PACKAGE "recorder")
set(RECORDER_PACKAGE_NAME "RECORDER")
set(RECORDER_PACKAGE_VERSION "${RECORDER_VERSION_MAJOR}.${RECORDER_VERSION_MINOR}.${RECORDER_VERSION_PATCH}")
set(RECORDER_PACKAGE_VERSION_MAJOR "${RECORDER_VERSION_MAJOR}.${RECORDER_VERSION_MINOR}")
set(RECORDER_PACKAGE_VERSION_MINOR "${RECORDER_VERSION_PATCH}")
set(RECORDER_PACKAGE_STRING "${RECORDER_PACKAGE_NAME} ${RECORDER_PACKAGE_VERSION}")
set(RECORDER_PACKAGE_TARNAME "${RECORDER_PACKAGE}")

#------------------------------------------------------------------------------
# Setup install and output Directories
#------------------------------------------------------------------------------
if(NOT RECORDER_INSTALL_BIN_DIR)
    set(RECORDER_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
endif()
if(NOT RECORDER_INSTALL_LIB_DIR)
    set(RECORDER_INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()
if(NOT RECORDER_INSTALL_INCLUDE_DIR)
    set(RECORDER_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
endif()
if(NOT RECORDER_INSTALL_DATA_DIR)
    set(RECORDER_INSTALL_DATA_DIR ${CMAKE_INSTALL_PREFIX}/share)
endif()

#------------------------------------------------------------------------------
# Setup CMake Environment
#------------------------------------------------------------------------------
if(APPLE)
    # We are doing a unix-style install i.e. everything will be installed in
    # CMAKE_INSTALL_PREFIX/bin and CMAKE_INSTALL_PREFIX/lib etc. as on other unix
    # platforms. We still need to setup CMAKE_INSTALL_NAME_DIR correctly so that
    # the binaries point to appropriate location for the libraries.

    # 1. Make CMAKE_INSTALL_PREFIX publicly accessible, if it was hidden in
    #    previous pass
    get_property(is_internal CACHE CMAKE_INSTALL_PREFIX PROPERTY TYPE)
    if(is_internal STREQUAL "INTERNAL")
        set(CMAKE_INSTALL_PREFIX ${CACHED_CMAKE_INSTALL_PREFIX} CACHE PATH "Install prefix" FORCE)
    else()
        set(CMAKE_INSTALL_PREFIX ${CACHED_CMAKE_INSTALL_PREFIX} CACHE PATH "Install prefix")
    endif()
    unset(MACOSX_APP_INSTALL_PREFIX CACHE)

    set(CMAKE_INSTALL_NAME_DIR "@rpath")
    mark_as_advanced(
            CMAKE_OSX_ARCHITECTURES
            CMAKE_OSX_DEPLOYMENT_TARGET
            CMAKE_OSX_SYSROOT
    )
endif()

if(NOT CMAKE_INSTALL_RPATH)
    set (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

if(NOT RECORDER_EXTERNALLY_CONFIGURED)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
            ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Executables."
            )
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
            ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Libraries"
            )
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
            ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all static libraries."
            )
endif()

set(RECORDER_CMAKE_DIR "${RECORDER_SOURCE_DIR}/CMake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${RECORDER_CMAKE_DIR})

#------------------------------------------------------------------------------
# Disallow in-source build
#------------------------------------------------------------------------------
if("${RECORDER_SOURCE_DIR}" STREQUAL "${RECORDER_BINARY_DIR}")
    message(FATAL_ERROR
            "RECORDER requires an out of source Build. "
            "Please create a separate binary directory and run CMake there.")
endif()

#------------------------------------------------------------------------------
# Set a default build type if none was specified
#------------------------------------------------------------------------------
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
            "MinSizeRel" "RelWithDebInfo")
endif()

#if(NOT "${CMAKE_CXX_STANDARD}")
#    set(CMAKE_CXX_STANDARD 17)
#    set(CMAKE_CXX_STANDARD_REQUIRED ON)
#    set(CMAKE_CXX_EXTENSIONS OFF)
#endif()

#-----------------------------------------------------------------------------
# Targets built within this project are exported at Install time for use
# by other projects.
#-----------------------------------------------------------------------------
if(NOT RECORDER_EXPORTED_TARGETS)
    set(RECORDER_EXPORTED_TARGETS "recorder-targets")
endif()

#------------------------------------------------------------------------------
# Build options
#------------------------------------------------------------------------------
option(BUILD_SHARED_LIBS "Build with shared libraries." ON)
option(RECORDER_ENABLE_PARQUET "Build Parquet format converter." OFF)
option(RECORDER_ENABLE_POSIX_TRACE "Enable tracing of POSIX calls." ON)
option(RECORDER_ENABLE_MPI_TRACE "Enable tracing of MPI communication calls." ON)
option(RECORDER_ENABLE_MPIIO_TRACE "Enable tracing of MPI-IO calls." ON)
option(RECORDER_ENABLE_HDF5_TRACE "Enable tracing of HDF5 calls." ON)
option(RECORDER_ENABLE_CUDA_TRACE "Enable tracing of CUDA kernels." OFF)
option(RECORDER_ENABLE_FCNTL_TRACE "Enable tracing of fcntl()." ON)
option(RECORDER_INSTALL_TESTS "Enable installation of tests." OFF)

#mark_as_advanced(RECORDER_ENABLE_POSIX_TRACE)
#mark_as_advanced(RECORDER_ENABLE_MPI_TRACE)
#mark_as_advanced(RECORDER_ENABLE_MPIIO_TRACE)
#mark_as_advanced(RECORDER_ENABLE_HDF5_TRACE)
#mark_as_advanced(RECORDER_ENABLE_FCNTL_TRACE)


if(BUILD_SHARED_LIBS)
    set(RECORDER_BUILD_SHARED_LIBS 1)
    set(RECORDER_LIBTYPE SHARED)
else()
    set(RECORDER_BUILD_SHARED_LIBS 0)
    set(RECORDER_LIBTYPE STATIC)
endif()


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fgnu89-inline -std=c99 -fcommon")
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")
endif()


if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (NOT "${CMAKE_CXX_FLAGS_DEBUG}" MATCHES ".*-g3.*")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3"
                CACHE STRING "" FORCE)
    endif()
    if (NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES ".*-g3.*")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3"
                CACHE STRING "" FORCE)
    endif()

endif()



#-------------------------------------------------------------------------------
function(recorder_set_lib_options libtarget libname libtype)
    if(${libtype} MATCHES "SHARED")
        if(WIN32 AND NOT MINGW)
            set(LIB_RELEASE_NAME "${libname}")
            set(LIB_DEBUG_NAME "${libname}_D")
            set(LIB_VERSION ${RECORDER_PACKAGE_VERSION_MAJOR})
        else()
            set(LIB_RELEASE_NAME "${libname}")
            set(LIB_DEBUG_NAME "${libname}_debug")
            set(LIB_VERSION ${RECORDER_PACKAGE_VERSION})
        endif()
    else()
        if(WIN32 AND NOT MINGW)
            set(LIB_RELEASE_NAME "lib${libname}")
            set(LIB_DEBUG_NAME "lib${libname}_D")
        else()
            # if the generator supports configuration types or if the CMAKE_BUILD_TYPE has a value
            if(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
                set(LIB_RELEASE_NAME "${libname}")
                set(LIB_DEBUG_NAME "${libname}_debug")
            else()
                set(LIB_RELEASE_NAME "lib${libname}")
                set(LIB_DEBUG_NAME "lib${libname}_debug")
            endif()
        endif()
    endif()

    set_target_properties(${libtarget}
            PROPERTIES
            DEBUG_OUTPUT_NAME          ${LIB_DEBUG_NAME}
            RELEASE_OUTPUT_NAME        ${LIB_RELEASE_NAME}
            MINSIZEREL_OUTPUT_NAME     ${LIB_RELEASE_NAME}
            RELWITHDEBINFO_OUTPUT_NAME ${LIB_RELEASE_NAME}
            VERSION                    ${LIB_VERSION}
            SOVERSION                  ${LIB_VERSION}
            )

    #----- Use MSVC Naming conventions for Shared Libraries
    if(MINGW AND ${libtype} MATCHES "SHARED")
        set_target_properties(${libtarget}
                PROPERTIES
                IMPORT_SUFFIX ".lib"
                IMPORT_PREFIX ""
                PREFIX ""
                )
    endif()
endfunction()

#-----------------------------------------------------------------------------
# Source
#-----------------------------------------------------------------------------
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tools)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tools/reporter)

#-----------------------------------------------------------------------------
# Testing
#-----------------------------------------------------------------------------
if(CMAKE_PROJECT_NAME STREQUAL RECORDER)
    include(CTest)
endif()

if(CMAKE_PROJECT_NAME STREQUAL RECORDER AND BUILD_TESTING)
    # include test sub directory
endif()

#-----------------------------------------------------------------------------
# Configure the config.cmake file for the build directory
#-----------------------------------------------------------------------------
configure_file(
        ${RECORDER_SOURCE_DIR}/CMake/recorder-config.cmake.build.in
        ${RECORDER_BINARY_DIR}/recorder-config.cmake @ONLY
)

#-----------------------------------------------------------------------------
# Configure the config.cmake file for the install directory
#-----------------------------------------------------------------------------
configure_file(
        ${RECORDER_SOURCE_DIR}/CMake/recorder-config.cmake.install.in
        ${RECORDER_BINARY_DIR}/CMakeFiles/recorder-config.cmake @ONLY
)

install(
        FILES
        ${RECORDER_BINARY_DIR}/CMakeFiles/recorder-config.cmake
        DESTINATION
        ${RECORDER_INSTALL_DATA_DIR}/cmake/recorder
)

#-----------------------------------------------------------------------------
# Configure the recorder-config-version .cmake file for the install directory
#-----------------------------------------------------------------------------
configure_file(
        ${RECORDER_SOURCE_DIR}/CMake/recorder-config-version.cmake.in
        ${RECORDER_BINARY_DIR}/CMakeFiles/recorder-config-version.cmake @ONLY
)

install(
        FILES
        ${RECORDER_BINARY_DIR}/CMakeFiles/recorder-config-version.cmake
        DESTINATION
        ${RECORDER_INSTALL_DATA_DIR}/cmake/recorder
)
