cmake_minimum_required(VERSION 3.2)

project(rustbindsettings C)

add_library(
    rustbindsettings
    STATIC
    libsettings_wrapper.h
  )

target_include_directories(rustbindsettings
    PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/../../third_party/libsbp/c/include/libsbp
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/../../include
  )

install(TARGETS rustbindsettings
    ARCHIVE DESTINATION lib
  )

set_target_properties(rustbindsettings PROPERTIES LINKER_LANGUAGE C)
