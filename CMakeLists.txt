cmake_minimum_required(VERSION 3.0)
project(web-server)

set(LIBCXX -stdlib=libc++)

# 3rdparties
find_package(Boost 1.52 REQUIRED COMPONENTS filesystem)
include_directories(${Boost_INCLUDE_DIR})

add_subdirectory(3rdparty/website)
target_compile_options(libwebsite PRIVATE -Wno-everything)
include_directories(3rdparty/website/include)

include_directories(3rdparty/json/include)

# Project
add_compile_options(
    -std=c++1y -Wall -Wextra
    ${LIBCXX}
)

add_subdirectory(src)

add_executable(${PROJECT_NAME} $<TARGET_OBJECTS:srclib>)

target_link_libraries(${PROJECT_NAME}
    Boost::filesystem
    libwebsite
)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS ${LIBCXX})

#add_executable(json-test json_load_test.cpp)
#target_link_libraries(json-test c++experimental)