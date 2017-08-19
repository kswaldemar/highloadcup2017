cmake_minimum_required(VERSION 3.0)
project(web-server)

set(LIBCXX -stdlib=libc++)

# 3rdparties
add_subdirectory(3rdparty/website)
target_compile_options(libwebsite PRIVATE -Wno-everything)
include_directories(3rdparty/website/include)

include_directories(3rdparty/json/include)

# Project
add_compile_options(
    -std=c++1z -Wall -Wextra
    ${LIBCXX}
)

add_subdirectory(src)

add_executable(${PROJECT_NAME} $<TARGET_OBJECTS:srclib>)

target_link_libraries(${PROJECT_NAME}
    c++experimental
    libwebsite
)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS ${LIBCXX})

#add_executable(json-test json_load_test.cpp)
#target_link_libraries(json-test c++experimental)