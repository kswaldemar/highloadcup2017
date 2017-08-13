cmake_minimum_required(VERSION 3.7)
project(web-server)

add_compile_options(
    -std=c++11
)

add_subdirectory(src)

add_executable(${PROJECT_NAME} 
    $<TARGET_OBJECTS:srclib>
)