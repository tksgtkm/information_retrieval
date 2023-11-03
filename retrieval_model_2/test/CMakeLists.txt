project(test)

add_executable(logging_test logging_test.cc)
add_executable(utils_test utils_test.cc)
add_executable(filesystem_test filesystem_test.cc)

target_link_libraries(logging_test misc)
target_link_libraries(utils_test misc)

set (FILESYSTEM_LIBS filesystem misc)
target_link_libraries(filesystem_test ${FILESYSTEM_LIBS})