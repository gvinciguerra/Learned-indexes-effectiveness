cmake_minimum_required(VERSION 3.8)
project(Simulations)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenMP)
if (OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif ()

include_directories(include)
add_executable(simulate simulate.cpp)
add_executable(segments_count segments_count.cpp)
add_executable(real_gaps real_gaps.cpp)