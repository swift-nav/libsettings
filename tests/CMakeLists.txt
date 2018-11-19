if (CMAKE_CROSSCOMPILING)
  message(STATUS "Skipping unit tests, cross compiling")
else (CMAKE_CROSSCOMPILING)
  ##################################
  # Download and install GoogleTest

  include(ExternalProject)
  ExternalProject_Add(gtest
    URL https://github.com/google/googletest/archive/release-1.8.1.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    INSTALL_COMMAND ""
  )
  enable_testing()
  find_package(GTest REQUIRED)

  ################
  # Define a test
  add_executable(
    test_libsettings
    src/test_registration_state.cpp
    ${PROJECT_SOURCE_DIR}/src/registration_state.c)
  add_dependencies(test_libsettings gtest)
  include_directories(${source_dir}/include)
  include_directories(${GTEST_INCLUDE_DIRS})
  include_directories(${PROJECT_SOURCE_DIR}/include)
  target_link_libraries(test_libsettings ${GTEST_BOTH_LIBRARIES} pthread)

  ################################
  # Unit Tests
  ################################
  add_custom_command(
      TARGET test_libsettings POST_BUILD
      COMMENT "Running unit tests"
      COMMAND test_libsettings
    )
endif (CMAKE_CROSSCOMPILING)
