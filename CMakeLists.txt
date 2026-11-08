cmake_minimum_required(VERSION 3.14)
project(LogProcessingFramework)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

# Libraries
add_library(log_processor src/log_processor.cpp src/inverted_index.cpp)

# Test Executables
add_executable(test_parser tests/test_parser.cpp)
target_link_libraries(test_parser PRIVATE log_processor)

add_executable(test_index tests/test_index.cpp)
target_link_libraries(test_index PRIVATE log_processor)