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

    add_subdirectory(archiever/thirdparty/dynamic-segment-tree)
    include_directories(archiever/thirdparty/dynamic-segment-tree/include)
    add_subdirectory(archiever/encoding_lib)
    include_directories(archiever/encoding_lib/include)
    add_subdirectory(archiever/applib)
    include_directories(archiever/applib/include)

    set(REC_CPPS
            lib/nj/nanojpeg.cpp lib/file_io.cpp
            lib/transform/dc_ac_transform.cpp)

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

endif()



