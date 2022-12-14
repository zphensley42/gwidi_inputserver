cmake_minimum_required(VERSION 3.23)
project(gwidi_inputserver)

set(CMAKE_CXX_STANDARD 17)

if(BUILD_TESTS)
    set(INPUTREADER_BUILD_TESTS ON)
    set(SENDINPUT_BUILD_TESTS ON)
    set(SOCKETSERVER_BUILD_TESTS ON)
endif()

if(NOT TARGET linux_inputreader)
    set(linux_inputreader_DIR ${CMAKE_CURRENT_LIST_DIR}/linux_inputreader)
    find_package(linux_inputreader REQUIRED)
endif()

if(NOT TARGET linux_sendinput)
    set(linux_sendinput_DIR ${CMAKE_CURRENT_LIST_DIR}/linux_sendinput)
    find_package(linux_sendinput REQUIRED)
endif()

if(NOT TARGET gwidi_socketserver)
    set(gwidi_socketserver_DIR ${CMAKE_CURRENT_LIST_DIR}/gwidi_socketserver)
    find_package(gwidi_socketserver REQUIRED)
endif()

if(NOT TARGET gwidi_server)
    set(gwidi_server_DIR ${CMAKE_CURRENT_LIST_DIR}/gwidi_server)
    find_package(gwidi_server REQUIRED)
endif()

add_executable(run_server run_server.cc)
target_include_directories(run_server PRIVATE ${linux_inputreader_INCLUDE_DIRS} ${linux_sendinput_INCLUDE_DIRS} ${gwidi_socketserver_INCLUDE_DIRS} ${gwidi_server_INCLUDE_DIRS})
target_link_libraries(run_server PRIVATE ${linux_inputreader_LIBRARIES} ${linux_sendinput_LIBRARIES} ${gwidi_socketserver_LIBRARIES} ${gwidi_server_LIBRARIES})

