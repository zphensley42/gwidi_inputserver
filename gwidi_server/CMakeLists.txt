cmake_minimum_required(VERSION 3.20.2)
project(gwidi_server)

set(CMAKE_CXX_STANDARD 17)

if(NOT TARGET gwidi_server)
    set(gwidi_server_DIR ${CMAKE_CURRENT_LIST_DIR})
    find_package(gwidi_server REQUIRED)
endif()
