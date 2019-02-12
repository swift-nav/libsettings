cmake_minimum_required(VERSION 3.1)


################################################################################
# Build Controls
################################################################################
# When not explicitly provided, default to building shared libraries where possible.
option(BUILD_SHARED_LIBS "If set, build shared libraries where possible." OFF)
# Enable Clang auto-formatting, defaults to on.
option(ENABLE_AUTOFORMAT "Enable automatic formatting and replacement with clang-format." ON)
# Enable Clang auto-linting, defaults to off.
option(ENABLE_AUTOLINT "Enable automatic linting and warning with clang-tidy. " OFF)
# Enable static stack analyzer, defaults to off.
option(ENABLE_STACK_ANALYSIS "Enable stack analysis. Requires gcc." OFF)

################################################################################
# Include Modules
################################################################################
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(ClangTools)

project(libsettings LANGUAGES C CXX)

################################################################################
# Targets
################################################################################

if (LIBSBP_SEARCH_PATH)
  include_directories("${LIBSBP_SEARCH_PATH}/include")
else (LIBSBP_SEARCH_PATH)
  if(EXISTS ${CMAKE_SOURCE_DIR}/libsbp/c)
    include_directories("${PROJECT_SOURCE_DIR}/libsbp/c/include")
  endif()
endif(LIBSBP_SEARCH_PATH)

include_directories("libswiftnav/include")

add_subdirectory(src)
add_subdirectory(python)
add_subdirectory(tests)
