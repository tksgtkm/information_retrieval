project(misc)

add_library(
  ${PROJECT_NAME}
  SHARED
  alloc.h
  alloc.cc
  # assert.h
  backend.h
  configrator.h
  configrator.cc
  io.h
  io.cc
  lockable.h
  lockable.cc
  logging.h
  logging.cc
  macros.h
  stringtokenizer.h
  stringtokenizer.cc
  utils.h
  utils.cc
)

target_sources(
  ${PROJECT_NAME}
  PRIVATE
    alloc.h
    alloc.cc
    # assert.h
    configrator.h
    configrator.cc
    io.h
    io.cc
    lockable.h
    lockable.cc
    logging.h
    logging.cc
    macros.h
    stringtokenizer.h
    stringtokenizer.cc
    utils.h
    utils.cc
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/backend.h
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(
  ${PROJECT_NAME} config
)

# pthreadをリンクする
target_link_libraries(
  ${PROJECT_NAME} "-pthread"
)
