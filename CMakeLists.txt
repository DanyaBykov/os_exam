cmake_minimum_required(VERSION 3.10)
project(mpi_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/src)

set(SRC
	src/main.cpp
	src/mympi.cpp
	src/integrate_parallel.cpp
	src/cfg_reader.cpp
	src/functions.cpp
)

add_executable(mpi_example ${SRC})
target_link_libraries(mpi_example PRIVATE Threads::Threads)