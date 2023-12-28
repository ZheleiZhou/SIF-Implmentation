# cmake_minimum_required (VERSION 2.8.11)
project (emp-agmpc)
set(NAME "emp-agmpc")

# find_path(CMAKE_FOLDER NAMES cmake/emp-tool-config.cmake)
# include(${CMAKE_FOLDER}/cmake/emp-base.cmake)

# FIND_PACKAGE(emp-ot REQUIRED)
# INCLUDE_DIRECTORIES(${EMP-OT_INCLUDE_DIRS})

add_library(emp-agmpc INTERFACE)

target_include_directories(emp-agmpc
INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(emp-agmpc
INTERFACE
  emp-ot
)


install(FILES cmake/emp-agmpc-config.cmake DESTINATION cmake/)
install(DIRECTORY emp-agmpc DESTINATION include/)

ENABLE_TESTING()
ADD_SUBDIRECTORY(test)
