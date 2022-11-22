if(NOT TARGET gwidi_socketserver)
    set(gwidi_socketserver_DIR ${CMAKE_CURRENT_LIST_DIR}/../)
    find_package(gwidi_socketserver REQUIRED)
endif()

add_executable(gwidi_socketserver_test main.cc)
target_include_directories(gwidi_socketserver_test PUBLIC
        ${gwidi_socketserver_INCLUDE_DIRS}
)
target_link_libraries(gwidi_socketserver_test PRIVATE ${gwidi_socketserver_LIBRARIES})
