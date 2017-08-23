cmake_minimum_required(VERSION 3.0)
project(web-server)

set(LIBCXX -stdlib=libc++)

# 3rdparties
include_directories(3rdparty/website/include)
link_directories(3rdparty/website/lib)
include_directories(3rdparty/json/include)

# Project
add_compile_options(
    -std=c++1z -Wall -Wextra
    -msse4.2 -march=native
    ${LIBCXX}
)

add_subdirectory(src)

add_executable(${PROJECT_NAME} $<TARGET_OBJECTS:srclib>)

target_link_libraries(${PROJECT_NAME}
    c++experimental
    website ev
)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS ${LIBCXX})
