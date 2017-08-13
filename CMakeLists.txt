cmake_minimum_required(VERSION 3.0)
project(web-server)

add_compile_options(
    -std=c++14 -Wall -Wextra
)

find_package(Boost 1.52 COMPONENTS system thread REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

include_directories(3rdparty/crow/include)

add_subdirectory(src)

add_executable(${PROJECT_NAME} $<TARGET_OBJECTS:srclib>)

target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    pthread
)