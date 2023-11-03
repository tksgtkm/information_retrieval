project(config)

add_library(
  ${PROJECT_NAME}
  INTERFACE
)
# add_library(sub::config ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# target_link_libraries(${PROJECT_NAME})
