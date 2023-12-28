add_library(mv-svole INTERFACE)

target_include_directories(mv-svole
INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(mv-svole
INTERFACE
  emp-zk
  emp-agmpc
)