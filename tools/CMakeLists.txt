#------------------------------------------------------------------------------
# Include source and build directories
#------------------------------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR}/include)

#------------------------------------------------------------------------------
# FLAGS for building
#------------------------------------------------------------------------------

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-result -std=c++11")

#------------------------------------------------------------------------------
# Tools
#------------------------------------------------------------------------------

add_library(reader reader.c)

add_executable(recorder2text recorder2text.c)
target_link_libraries(recorder2text reader)
add_dependencies(recorder2text reader)

add_executable(metaops_checker meta_ops_checker.c)
target_link_libraries(metaops_checker reader)
add_dependencies(metaops_checker reader)

add_executable(conflict_detector overlap_conflict.c build_offset_intervals.cpp)
target_link_libraries(conflict_detector reader)
add_dependencies(conflict_detector reader)

if(RECORDER_ENABLE_PARQUET)
    find_package(Arrow REQUIRED)
    if(${Arrow_FOUND})
        message(info "Arrow found")
        include_directories(${ARROW_INCLUDE_DIR})
    else()
        message(info "Arrow not found")
    endif()
    find_package(Parquet REQUIRED)
    if(${Parquet_FOUND})
        message(info "Parquet found")
        include_directories(${PARQUET_INCLUDE_DIR})
    else()
        message(info "Parquet not found")
    endif()

    add_executable(recorder2parquet recorder2parquet.cpp)
    target_link_libraries(recorder2parquet reader arrow_shared parquet_shared)
    add_dependencies(recorder2parquet reader)
    install(
        TARGETS
        recorder2parquet 
        EXPORT
        ${RECORDER_EXPORTED_TARGETS}
        LIBRARY DESTINATION ${RECORDER_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${RECORDER_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${RECORDER_INSTALL_BIN_DIR}
    )
endif()



#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install
#-----------------------------------------------------------------------------
set(targets reader recorder2text metaops_checker conflict_detector)
foreach(target ${targets})
    install(
        TARGETS
        ${target} 
        EXPORT
        ${RECORDER_EXPORTED_TARGETS}
        LIBRARY DESTINATION ${RECORDER_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${RECORDER_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${RECORDER_INSTALL_BIN_DIR}
    )
endforeach()

