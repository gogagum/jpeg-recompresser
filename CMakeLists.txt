cmake_minimum_required(VERSION 3.5)

project(draft-enc LANGUAGES CXX)
project(draft-dec LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost COMPONENTS system REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(draft-enc draft/lib/jpegenc.cpp draft/lib/jo_write_jpeg.cpp)
    add_executable(draft-dec draft/lib/jpegdec.cpp)

    install(TARGETS draft-enc draft-dec
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()



