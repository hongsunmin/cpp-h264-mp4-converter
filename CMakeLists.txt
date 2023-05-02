cmake_minimum_required(VERSION 3.13)
project(h264-mp4-converter)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpermissive")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread")

add_subdirectory(libmp4v2)

add_library(h264-mp4-converter-library
    h264-mp4-converter.cpp
)

target_include_directories(h264-mp4-converter-library PRIVATE
    "libmp4v2/include"
    "libmp4v2"
    ${CMAKE_CURRENT_BINARY_DIR}/libmp4v2/include
)

add_executable(h264-mp4-converter-test
    main.cpp
)

target_link_libraries(h264-mp4-converter-test h264-mp4-converter-library mp4v2)
