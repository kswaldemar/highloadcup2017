cmake_minimum_required(VERSION 3.0)

set(sources
    main.cpp
    SimpleDB.cpp
    PodTypes.cpp
    WebServer.cpp
)

add_library(srclib OBJECT ${sources})