project(index LANGUAGES CXX)

add_library(
  ${PROJECT_NAME}
  SHARED
  index_types.h
  index_types.cc
  ondisk_index.h
  compactindex.h
  compactindex.cc
)

target_sources(
  ${PROJECT_NAME}
  PRIVATE
    index_types.h
    index_types.cc
    compactindex.h
    compactindex.cc
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/index_types.h
    ${CMAKE_CURRENT_LIST_DIR}/compactindex.h
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(
  ${PROJECT_NAME} config
)

target_link_libraries(
  index
)