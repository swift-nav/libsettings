cmake_minimum_required(VERSION 3.2)

add_library(
   rustbindsettings
   STATIC
   libsettings_wrapper.c
)

target_include_directories(rustbindsettings
    PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/../..
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/../../third_party/libsbp/c/include
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/../../include
)

install(TARGETS rustbindsettings
  ARCHIVE DESTINATION lib
  )
