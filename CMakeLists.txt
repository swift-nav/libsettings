cmake_minimum_required(VERSION 3.2)

################################################################################
# Build Controls
################################################################################
# Enable Clang auto-formatting, defaults to on.
option(ENABLE_AUTOFORMAT "Enable automatic formatting and replacement with clang-format." ON)
# Enable Clang auto-linting, defaults to off.
option(ENABLE_AUTOLINT "Enable automatic linting and warning with clang-tidy. " OFF)
# Enable static stack analyzer, defaults to off.
option(ENABLE_STACK_ANALYSIS "Enable stack analysis. Requires gcc." OFF)

project(libsettings LANGUAGES C CXX)

################################################################################
# Dependency Paths
# If you wish to use versions other than what's bundled with libsettings
# set the variables to the path to the root of the different repo version
################################################################################
if (NOT LIBSBP_PATH)
    set(LIBSBP_PATH "${PROJECT_SOURCE_DIR}/libsbp/")
endif()
message(STATUS "Using LibSbp in ${LIBSBP_PATH}")

if (NOT LIBSWIFTNAV_PATH)
    set(LIBSWIFTNAV_PATH "${PROJECT_SOURCE_DIR}/libswiftnav/")
endif()
message(STATUS "Using LibSwiftNav in ${LIBSWIFTNAV_PATH}")

################################################################################
# Include Modules
################################################################################
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(ClangTools)

################################################################################
# Targets
################################################################################

add_subdirectory(src)
add_subdirectory(python)
add_subdirectory(tests)
