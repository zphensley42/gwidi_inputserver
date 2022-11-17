if(NOT TARGET linux_inputreader)
    set(linux_inputreader_DIR ${CMAKE_CURRENT_LIST_DIR}/../)
    find_package(linux_inputreader REQUIRED)
endif()

add_executable(linux_inputreader_test main.cc)
target_include_directories(linux_inputreader_test PUBLIC
        ${linux_inputreader_INCLUDE_DIRS}
)
target_link_libraries(linux_inputreader_test PRIVATE ${linux_inputreader_LIBRARIES})
