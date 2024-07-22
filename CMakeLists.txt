# Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
#
# FLM SDK
#

cmake_minimum_required(VERSION 3.10)

if(POLICY CMP0091)
   cmake_policy(SET CMP0091 NEW)
endif()

if(POLICY CMP0076)
  cmake_policy(SET CMP0076 NEW)
endif()

project(FLM)

# ------------------------------
# Helper function for the build 
# ------------------------------
include(cmake/helperfunctions.cmake)

# -----------------------------------
# Required for FLM
# -----------------------------------
find_package(Threads)

# -----------------------------------
# Set standard to c++17
# -----------------------------------
set(CMAKE_CXX_STANDARD 17)
add_compile_definitions(_CMP_CPP17_=1)
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT flm)

# -----------------------------------------------------------
# enable multi-threaded compilation
# -----------------------------------------------------------
add_compile_options(/MP)
add_compile_definitions(API_DX12)

# -----------------------------------------------------------
# Check for Visual Studio build tooling
# -----------------------------------------------------------
if(MSVC_TOOLSET_VERSION VERSION_LESS 142)
    message(FATAL_ERROR "Cannot find MSVC toolset version 142 or greater. Please make sure Visual Studio 2019 or newer installed")
endif()

# -----------------------------------------------------------
# generate the output binary in the /bin directory
# -----------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/win/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/win/bin/lib)   # Dynamic DLL's 
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/win/bin/lib)   # Static Libs

# -----------------------------------------------------------
# Helper for copy files 
# -----------------------------------------------------------
macro(FLM_copy_to_output executable source dest)
    add_custom_command(TARGET ${executable} PRE_BUILD
        # COMMENT "Copying ${source} -> ${dest}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                ${source}
                ${dest}
    )
endmacro()


# -----------------------------------------------------------
# Use solution folders.
# -----------------------------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# -----------------------------------------------------------
# CLI Application
# -----------------------------------------------------------
add_subdirectory(source/flm_cli)

# -----------------------------------------------------------
# Backend Lib
# -----------------------------------------------------------
add_subdirectory(source/flm_backend)

