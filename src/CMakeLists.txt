cmake_minimum_required(VERSION 3.0)

# Link with libc++ instead of libstdc++ for experimental features
set(LIBCXX -stdlib=libc++)

add_compile_options(
    -std=c++1z -Wall -Wextra
    -msse4.2 -march=native
    ${LIBCXX}
)

# Executable
set(Sources
    main.cpp
    SimpleDB.cpp
    PodTypes.cpp
    RequestHandler.cpp)

set(Headers
    SimpleLog.h
    urldecode.h)

add_executable(${PROJECT_NAME} ${Sources} ${Headers})
target_link_libraries(${PROJECT_NAME}
    c++experimental
    libwebsite
    jemalloc)

set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS ${LIBCXX}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Configure server-run.sh
configure_file(${CMAKE_SOURCE_DIR}/scripts/server-run.sh.in ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/server-run.sh @ONLY)
file(COPY ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/server-run.sh
    DESTINATION ${CMAKE_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE)