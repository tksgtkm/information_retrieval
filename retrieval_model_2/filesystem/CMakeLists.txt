project(filesystem LANGUAGES CXX)

add_library(
  ${PROJECT_NAME}
  SHARED
  filesystem.h
  filesystem.cc
  filesystemcache.cc
  fileobject.cc
)

target_sources(
  ${PROJECT_NAME}
  PRIVATE
    filesystem.h
    filesystem.cc
    filesystemcache.cc
    fileobject.cc
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/filesystem.h
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

set(LIBS misc index)

target_link_libraries(
  ${PROJECT_NAME} ${LIBS}
)

target_link_libraries(
  filesystem
)