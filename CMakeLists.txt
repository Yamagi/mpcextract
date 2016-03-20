cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

# Enforce 'Debug' as default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# Project configuration
project(mpcexctract)

# Base compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror -pedantic")

# Source files
set(SOURCE_FILES src/mpcextract.cpp)

# Executable 'mpcextract'
add_executable(mpcextract ${SOURCE_FILES})
