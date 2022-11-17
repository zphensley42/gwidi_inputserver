cmake_minimum_required(VERSION 3.20.2)
project(linux_inputreader)

set(CMAKE_CXX_STANDARD 17)

if(NOT TARGET linux_inputreader)
    set(linux_inputreader_DIR ${CMAKE_CURRENT_LIST_DIR})
    find_package(linux_inputreader REQUIRED)
endif()

if(INPUTREADER_BUILD_TESTS)
    message("BUILDING TESTS")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()
