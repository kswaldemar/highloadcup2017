cmake_minimum_required(VERSION 3.0)
project(web-server)

include(cmake/LoadDependency.cmake)

# 3rdparties
LoadDependency(https://github.com/nlohmann/json/releases/download/v2.1.1/json.hpp 3rdparty/json/include/json.hpp)
include_directories(3rdparty/json/include)

add_subdirectory(3rdparty/libwebsite)

# Project
add_subdirectory(src)
