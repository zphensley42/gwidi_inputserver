add_library(linux_sendinput)
target_sources(linux_sendinput PUBLIC ${CMAKE_CURRENT_LIST_DIR}/LinuxSendInput.cc)
target_include_directories(linux_sendinput PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

set(linux_sendinput_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
set(linux_sendinput_LIBRARIES linux_sendinput)

if(SENDINPUT_BUILD_TESTS)
    message("BUILDING TESTS")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()
