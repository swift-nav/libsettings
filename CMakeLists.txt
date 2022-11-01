cmake_minimum_required(VERSION 2.8.7)
project(libsettings)

################################################################################
# Build Controls
################################################################################
# Only apply defaults when this is the top level project
################################################################################
if(${PROJECT_NAME} STREQUAL ${CMAKE_PROJECT_NAME})
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
  # Enable static stack analyzer, defaults to off.
  option(ENABLE_STACK_ANALYSIS "Enable stack analysis. Requires gcc." OFF)
  # Selectively enable python support, defaults to on.
  option(libsettings_ENABLE_PYTHON "Enable building of python module" ON)
endif()

################################################################################
# Include Modules
################################################################################
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common")
include(GNUInstallDirs)
include(CCache)
include(SwiftCmakeOptions)

if(MSVC)
  message(STATUS "Skipping unit tests, MSVC detected")
  set(disable_tests TRUE)
elseif(SKIP_UNIT_TESTS)
  message(STATUS "Skipping unit tests requested")
  set(disable_tests TRUE)
else()
  set(disable_tests FALSE)
endif()

swift_create_project_options(
    HAS_TESTS
    DISABLE_TEST_COMPONENTS ${disable_tests}
    TEST_PACKAGES "Googletest"
    )

include(ClangFormat)
swift_setup_clang_format()

include(OldClangTidy)
swift_setup_clang_tidy(PATTERNS "src/*.c")

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

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-warning-option -Wno-error=typedef-redefinition")

  if (CMAKE_C_FLAGS MATCHES "-fsanitize=")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize-blacklist=${PROJECT_SOURCE_DIR}/sanitize-ignorelist.txt")
  endif()
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if (CMAKE_CXX_FLAGS MATCHES "-fsanitize=")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-blacklist=${PROJECT_SOURCE_DIR}/sanitize-ignorelist.txt")
  endif()
endif()

if (ENABLE_STACK_ANALYSIS AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  message(STATUS "Enabling stack analysis.")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-usage -fstack-check")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-usage -fstack-check")
endif ()

################################################################################
# Targets
################################################################################

option(libsbp_ENABLE_TESTS "" OFF)
option(libsbp_ENABLE_DOCS "" OFF)
find_package(Sbp REQUIRED)

option(libswiftnav_ENABLE_TESTS "" OFF)
option(libswiftnav_ENABLE_TEST_LIBS "" OFF)
find_package(Swiftnav REQUIRED)

add_subdirectory(src)
if(libsettings_ENABLE_PYTHON)
  add_subdirectory(python)
endif()

if(libsettings_BUILD_TESTS)
  add_subdirectory(tests)
endif()
