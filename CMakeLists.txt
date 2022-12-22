cmake_minimum_required(VERSION 3.5)

project(draft-enc LANGUAGES CXX)
project(draft-dec LANGUAGES CXX)

project(jpeg-recompress LANGUAGES CXX)
project(jpeg-recompress-back LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost COMPONENTS system REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(draft-enc draft/lib/jpegenc.cpp
                   draft/lib/enc/jo_write_jpeg.cpp
                   draft/lib/enc/jo_write_jpeg.hpp)
    add_executable(draft-dec draft/lib/jpegdec.cpp draft/lib/dec/nanojpeg.cpp)

    add_executable(jpeg-recompress jpeg_recompress.cpp
                   lib/nj/nanojpeg.hpp
                   lib/nj/nanojpeg.cpp
                   lib/file_io.cpp
                   lib/magical/process.hpp
                   archiever/include/arithmetic_coder.hpp
                   archiever/include/arithmetic_coder_encoded.hpp
                   archiever/include/arithmetic_coder_encoded.cpp)


    add_executable(jpeg_recompress_back jpeg_recompress_back.cpp
                   lib/file_io.cpp
                   lib/magical/process_back.cpp
                   archiever/include/arithmetic_decoder_decoded.hpp
                   archiever/include/arithmetic_decoder_decoded.cpp
                   lib/jo/jo_write_jpeg.cpp)

    install(TARGETS jpeg-recompress jpeg_recompress_back
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()



