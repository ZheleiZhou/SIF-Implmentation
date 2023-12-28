add_library(sif INTERFACE)

target_include_directories(sif
INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(sif
INTERFACE
  mv-svole
)