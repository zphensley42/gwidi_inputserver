if(NOT TARGET linux_sendinput)
    set(linux_sendinput_DIR ${CMAKE_CURRENT_LIST_DIR}/../)
    find_package(linux_sendinput REQUIRED)
endif()

add_executable(linux_sendinput_test main.cc)
target_include_directories(linux_sendinput_test PUBLIC
        ${linux_sendinput_INCLUDE_DIRS}
)
target_link_libraries(linux_sendinput_test PRIVATE ${linux_sendinput_LIBRARIES})
