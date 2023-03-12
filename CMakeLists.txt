cmake_minimum_required(VERSION 3.15)

project(jpeg-recompress LANGUAGES CXX)
project(jpeg-recompress-back LANGUAGES CXX)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

if (CMAKE_CROSSCOMPILING)
    message("Cross compiling.")
    message("Compile target: ${CMAKE_CXX_COMPILER_TARGET}")
    if (${CMAKE_CXX_COMPILER_TARGET} STREQUAL "x86_64-w64-mingw32")
        set(BOOST_ROOT "/usr/x86_64-w64-mingw32/lib/cmake/Boost-1.81.0")
        set(BOOST_INCLUDEDIR "/usr/x86_64-w64-mingw32/include")
        set(BOOST_LIBRARYDIR "/usr/x86_64-w64-mingw32/lib")
    elseif(${CMAKE_CXX_COMPILER_TARGET} STREQUAL "i686-w64-mingw32")
        set(BOOST_ROOT "/usr/i686-w64-mingw32/lib/cmake/Boost-1.81.0")
        set(BOOST_INCLUDEDIR "/usr/i686-w64-mingw32/include")
        set(BOOST_LIBRARYDIR "/usr/i686-w64-mingw32/lib")
    else()
        message( FATAL_ERROR "Can not cross compile for target ${CMAKE_CXX_COMPILER_TARGET}." )
    endif()
else()
    message("Not cross compiling.")
    message("Compile target: ${CMAKE_CXX_COMPILER_TARGET}")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")

add_subdirectory(archiever/thirdparty/dynamic-segment-tree)
include_directories(archiever/thirdparty/dynamic-segment-tree/include)
add_subdirectory(archiever/encoding_lib)
include_directories(archiever/encoding_lib/include)
add_subdirectory(archiever/applib)
include_directories(archiever/applib/include)

set(REC_CPPS
        lib/nj/nanojpeg.cpp lib/file_io.cpp lib/transform/dc_ac_transform.cpp)

add_executable(jrec jrec.cpp ${REC_CPPS})
target_link_libraries(jrec archievers-applib arithmetic-encoding-lib)

add_executable(jrec-swch jrec_swch.cpp ${REC_CPPS})
target_link_libraries(jrec-swch archievers-applib arithmetic-encoding-lib)

add_executable(jrec-swch-he jrec_swch_he.cpp ${REC_CPPS})
target_link_libraries(jrec-swch-he archievers-applib arithmetic-encoding-lib)

set(REC_CPPS_BACK 
        lib/nj/nanojpeg.cpp lib/file_io.cpp
        lib/jo/jo_write_jpeg.cpp lib/transform/dc_ac_transform.cpp)

add_executable(jrec-back jrec_back.cpp ${REC_CPPS_BACK})
target_link_libraries(jrec-back archievers-applib arithmetic-encoding-lib)

add_executable(jrec-back-swch jrec_back_swch.cpp ${REC_CPPS_BACK})
target_link_libraries(jrec-back-swch archievers-applib arithmetic-encoding-lib)

add_executable(jrec-back-swch-he jrec_back_swch_he.cpp ${REC_CPPS_BACK})
target_link_libraries(jrec-back-swch-he archievers-applib arithmetic-encoding-lib)
