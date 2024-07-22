#=============================================================================
# Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
#  @author AMD Developer Tools Team
#  @file CMakeLists.txt
#  @brief  FLM CLI CMakeLists file.
#=============================================================================

cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0091 NEW)


set(CMAKE_POLICY_DEFAULT_CMP0091 NEW) 
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL") # sets multi-threaded dynamically-linked runtime library

link_directories( 
    ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}
    ) 

set(FLM_SOURCE_MAIN
    main.h
    main.cpp
    flm_help.h
    ${PROJECT_SOURCE_DIR}/source/flm_backend/version.h
)

file(GLOB_RECURSE RESOURCES
    resources/*
)

# setup target binary
add_executable(flm
    ${FLM_SOURCE_MAIN}
    ${RESOURCES}
    )

source_group("source"          FILES ${FLM_SOURCE_MAIN})
source_group("resource"        FILES ${RESOURCES})

target_include_directories(flm PUBLIC
    ./
    ${PROJECT_SOURCE_DIR}/source/flm_backend
    ${PROJECT_SOURCE_DIR}/external/amf/amf
)

# add link time dependencies
target_link_libraries(flm  PRIVATE 
    flm_backend$<$<CONFIG:Debug>:d>
    )

add_dependencies(flm 
    flm_backend
    )

set_target_properties(flm PROPERTIES 
        FOLDER "application"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>" # host applications working folder
)