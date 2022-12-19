if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

if(NOT TARGET linux_sendinput)
    set(linux_sendinput_DIR ${CMAKE_CURRENT_LIST_DIR}/../linux_sendinput/)
    find_package(linux_sendinput REQUIRED)
endif()

add_library(gwidi_socketserver)
target_sources(gwidi_socketserver PUBLIC ${CMAKE_CURRENT_LIST_DIR}/GwidiSocketServer.cc)
target_link_libraries(gwidi_socketserver PUBLIC spdlog::spdlog ${linux_sendinput_LIBRARIES})
target_include_directories(gwidi_socketserver PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include ${linux_sendinput_INCLUDE_DIRS})

set(gwidi_socketserver_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
set(gwidi_socketserver_LIBRARIES gwidi_socketserver)

if(SOCKETSERVER_BUILD_TESTS)
    message("BUILDING TESTS")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()
