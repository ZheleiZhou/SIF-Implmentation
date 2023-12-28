cmake_minimum_required (VERSION 3.0)
project (emp-ot)
set(NAME "emp-ot")

# find_path(CMAKE_FOLDER NAMES cmake/emp-tool-config.cmake)
# include(${CMAKE_FOLDER}/cmake/emp-base.cmake)

# find_package(emp-tool REQUIRED)
include_directories(${EMP_TOOL_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${EMP_OT_INCLUDE_DIR})

add_library(emp-ot INTERFACE)

target_include_directories(emp-ot
INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(emp-ot
INTERFACE
  emp-tool
)


# Installation
install(FILES cmake/emp-ot-config.cmake DESTINATION cmake/)
install(DIRECTORY emp-ot DESTINATION include/)

ENABLE_TESTING()
ADD_SUBDIRECTORY(test)
