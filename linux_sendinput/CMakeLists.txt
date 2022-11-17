cmake_minimum_required(VERSION 3.20.2)
project(linux_sendinput)

set(CMAKE_CXX_STANDARD 17)

if(NOT TARGET linux_sendinput)
    set(linux_sendinput_DIR ${CMAKE_CURRENT_LIST_DIR})
    find_package(linux_sendinput REQUIRED)
endif()

if(SENDINPUT_BUILD_TESTS)
    message("BUILDING TESTS")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()
