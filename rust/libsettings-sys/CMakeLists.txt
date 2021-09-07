cmake_minimum_required(VERSION 3.2)

add_library(
   rustbindsettings
   INTERFACE
)

target_include_directories(rustbindsettings
    INTERFACE
    .
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/../../third_party/libsbp/c/include/libsbp
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/../../include
)

install(TARGETS rustbindsettings
  ARCHIVE DESTINATION lib
  )
