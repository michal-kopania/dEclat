cmake_minimum_required(VERSION 3.18)
project(dEclat)

set(CMAKE_CXX_STANDARD 17)
add_executable(dEclat main.cpp tree.cxx tree.hpp)

find_package(Boost REQUIRED COMPONENTS system program_options)

#INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
#link_directories(${Boost_LIBRARY_DIRS})
#target_link_libraries(dEclat ${Boost_LIBRARIES})

target_link_libraries(dEclat Boost::program_options)
