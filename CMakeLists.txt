cmake_minimum_required(VERSION 3.0)
project(web-server)

include(cmake/LoadDependencies.cmake)
CheckAndLoadDependencies()

# 3rdparties
include_directories(3rdparty/json/include)
add_subdirectory(3rdparty/libwebsite)

# Project
add_subdirectory(src)
