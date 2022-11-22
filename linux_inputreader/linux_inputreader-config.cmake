if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

if(NOT TARGET X11)
    find_package(X11 REQUIRED)
endif()

if(NOT TARGET gwidi_socketserver)
    set(gwidi_socketserver_DIR ${CMAKE_CURRENT_LIST_DIR}/../gwidi_socketserver)
    find_package(gwidi_socketserver REQUIRED)
endif()

add_library(linux_inputreader)
target_sources(linux_inputreader PUBLIC ${CMAKE_CURRENT_LIST_DIR}/LinuxInputReader.cc)
target_link_libraries(linux_inputreader PUBLIC spdlog::spdlog ${gwidi_socketserver_LIBRARIES} ${X11_LIBRARIES})
target_include_directories(linux_inputreader PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

set(linux_inputreader_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
set(linux_inputreader_LIBRARIES linux_inputreader)

if(INPUTREADER_BUILD_TESTS)
    message("BUILDING TESTS")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()
