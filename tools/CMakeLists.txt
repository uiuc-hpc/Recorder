#------------------------------------------------------------------------------
# Include source and build directories
#------------------------------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR}/include)


#------------------------------------------------------------------------------
# FLAGS for building
#------------------------------------------------------------------------------
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-result -std=c++11")


find_package(MPI REQUIRED)
if(MPI_FOUND)
    include_directories(${MPI_CXX_INCLUDE_DIRS})
    set(RECORDER_EXT_INCLUDE_DEPENDENCIES ${MPI_C_INCLUDE_DIRS}
            ${RECORDER_EXT_INCLUDE_DEPENDENCIES}
            )
    set(RECORDER_EXT_LIB_DEPENDENCIES
            ${MPI_C_LIBRARIES} ${RECORDER_EXT_LIB_DEPENDENCIES})
else()
    message(STATUS, "MPI not found")
endif()




#------------------------------------------------------------------------------
# Tools
#------------------------------------------------------------------------------


add_library(reader reader.c)

add_executable(recorder2text recorder2text.c)
target_link_libraries(recorder2text
                        PUBLIC ${MPI_C_LIBRARIES}
                        PUBLIC m
                        reader
                     )
add_dependencies(recorder2text reader)


add_executable(recorder2timeline recorder2timeline.cpp)
target_link_libraries(recorder2timeline
        PUBLIC ${MPI_CXX_LIBRARIES} reader)
add_dependencies(recorder2timeline reader)


#add_executable(metaops_checker meta_ops_checker.c)
#target_link_libraries(metaops_checker reader)
#add_dependencies(metaops_checker reader)

add_executable(conflict_detector conflict_detector.cpp build_offset_intervals.cpp)
target_link_libraries(conflict_detector reader)
add_dependencies(conflict_detector reader)

add_executable(recorder_summary recorder_summary.c)
target_link_libraries(recorder_summary reader)
add_dependencies(recorder_summary reader)


if(RECORDER_ENABLE_PARQUET)
    message("-- " "Configuring Parquet tool: TRUE")
    find_package(Arrow REQUIRED)
    if(${Arrow_FOUND})
        message("-- " "Found Arrow: TRUE")
        include_directories(${ARROW_INCLUDE_DIR})
    else()
        message("-- " "Found Arrow: FALSE")
    endif()
    find_package(Parquet REQUIRED)
    if(${Parquet_FOUND})
        message("-- " "Found Parquet: TRUE")
        include_directories(${PARQUET_INCLUDE_DIR})
    else()
        message("-- " "Found Parquet: FALSE")
    endif()

    add_executable(recorder2parquet recorder2parquet.cpp)
    target_link_libraries(recorder2parquet
                          PUBLIC ${MPI_C_LIBRARIES}
                          reader
                          arrow_shared
                          parquet_shared
                         )
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
#set(targets reader recorder2text metaops_checker conflict_detector)
set(targets reader recorder2text recorder2timeline conflict_detector recorder_summary)
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

