if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

add_library(gwidi_socketserver)
target_sources(gwidi_socketserver PUBLIC ${CMAKE_CURRENT_LIST_DIR}/GwidiSocketServer.cc)
target_link_libraries(gwidi_socketserver PUBLIC spdlog::spdlog)
target_include_directories(gwidi_socketserver PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

set(gwidi_socketserver_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
set(gwidi_socketserver_LIBRARIES gwidi_socketserver)

if(SOCKETSERVER_BUILD_TESTS)
    message("BUILDING TESTS")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()
