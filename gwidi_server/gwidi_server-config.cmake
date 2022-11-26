if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

if(NOT TARGET gwidi_socketserver)
    set(gwidi_socketserver_DIR ${CMAKE_CURRENT_LIST_DIR}/../gwidi_socketserver/)
    find_package(gwidi_socketserver REQUIRED)
endif()

if(NOT TARGET linux_inputreader)
    set(linux_inputreader_DIR ${CMAKE_CURRENT_LIST_DIR}/../linux_inputreader/)
    find_package(linux_inputreader REQUIRED)
endif()

add_library(gwidi_server)
target_sources(gwidi_server PUBLIC ${CMAKE_CURRENT_LIST_DIR}/GwidiServer.cc)
target_link_libraries(gwidi_server PUBLIC spdlog::spdlog gwidi_socketserver linux_inputreader)
target_include_directories(gwidi_server PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

set(gwidi_server_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
set(gwidi_server_LIBRARIES gwidi_server)
