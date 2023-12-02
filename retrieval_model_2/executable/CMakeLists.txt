project(executable)

add_executable(cli_system cli_system.cc)
add_executable(whs whs.cc)
add_executable(trec trec.cc)

target_link_libraries(cli_system misc)
target_link_libraries(whs misc)
target_link_libraries(trec config misc index)