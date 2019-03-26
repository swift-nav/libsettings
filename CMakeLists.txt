cmake_minimum_required(VERSION 2.8.7)
project(libsettings)

################################################################################
# Include Modules
################################################################################
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(GNUInstallDirs)
find_package(sbp REQUIRED)
find_package(swiftnav REQUIRED)
find_package(gtest REQUIRED)

################################################################################
# Build Controls
################################################################################
# When not explicitly provided, default to building shared libraries where possible.
set(BUILD_SHARED_LIBS ON CACHE BOOL "If set, build shared libraries where possible.")
# When not explicitly provided, default to debug release build.
#
# NOTE: CMake project generation sets the cache variable to the empty string on
#       initialization. In order to account for this, it is  necessary to use the
#       explicit 'if' statement in conjunction with the 'FORCE' directive.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
        "Choose one: None Debug Release RelWithDebInfo MinSizeRel Coverage." FORCE)
endif(NOT CMAKE_BUILD_TYPE)
# Enable Clang auto-formatting, defaults to on.
option(ENABLE_AUTOFORMAT "Enable automatic formatting and replacement with clang-format." ON)
# Enable Clang auto-linting, defaults to off.
option(ENABLE_AUTOLINT "Enable automatic linting and warning with clang-tidy. " OFF)
# Enable static stack analyzer, defaults to off.
option(ENABLE_STACK_ANALYSIS "Enable stack analysis. Requires gcc." OFF)

################################################################################
# Source Code Configuration
################################################################################

################################################################################
# Third party.
################################################################################

################################################################################
# Compilation Settings
################################################################################
# Build-type dependent flags.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -s")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -s")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
set(CMAKE_CXX_STANDARD 11)

# Setup flags for Code Coverage build mode
set(CMAKE_C_FLAGS_COVERAGE "${CMAKE_C_FLAGS_DEBUG} --coverage" CACHE STRING
    "Flags used by the C compiler for building with code coverage."
    FORCE)
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE
    "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage" CACHE STRING
    "Flags used for linking binaries with code coverage."
    FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE
    "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} --coverage" CACHE STRING
    "Flags used by the shared libraries linker during builds with code coverage."
    FORCE)
mark_as_advanced(
    CMAKE_CXX_FLAGS_COVERAGE
    CMAKE_C_FLAGS_COVERAGE
    CMAKE_EXE_LINKER_FLAGS_COVERAGE
    CMAKE_SHARED_LINKER_FLAGS_COVERAGE)

# Set project version using Git tag and hash.
execute_process(
  COMMAND git describe --dirty --tags --always
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE GIT_VERSION_FOUND
  ERROR_QUIET
  OUTPUT_VARIABLE GIT_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (GIT_VERSION_FOUND)
  set(VERSION "unknown")
else (GIT_VERSION_FOUND)
  set(VERSION ${GIT_VERSION})
endif (GIT_VERSION_FOUND)

# Some compiler options used globally
if (NOT MSVC)
  set(CMAKE_C_FLAGS
      "-Wall -Wextra -Wno-strict-prototypes -Werror -fno-unwind-tables \
       -fno-asynchronous-unwind-tables -Wimplicit -Wshadow -Wswitch-default \
       -Wswitch-enum -Wundef -Wuninitialized -Wcast-align -Wformat=2 \
       -Wimplicit-function-declaration -Wredundant-decls -Wformat-security \
       -ggdb ${CMAKE_C_FLAGS}")
endif (NOT MSVC)

if (CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-warning-option -Wno-error=typedef-redefinition")
endif()

if (ENABLE_STACK_ANALYSIS AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  message(STATUS "Enabling stack analysis.")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-usage -fstack-check")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-usage -fstack-check")
endif ()

################################################################################
# Targets
################################################################################

add_subdirectory(src)
add_subdirectory(python)
add_subdirectory(tests)
