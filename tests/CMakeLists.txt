if (CMAKE_CROSSCOMPILING)
  message(STATUS "Skipping unit tests, cross compiling")
else (CMAKE_CROSSCOMPILING)
  enable_testing()

  ############
  # Prepare gtest
  add_subdirectory("${PROJECT_SOURCE_DIR}/thirdparty/googletest" "thirdparty/googletest")

  mark_as_advanced(
    BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
    gmock_build_tests gtest_build_samples gtest_build_tests
    gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
  )

  set_target_properties(gtest PROPERTIES FOLDER extern)
  set_target_properties(gtest_main PROPERTIES FOLDER extern)
  set_target_properties(gmock PROPERTIES FOLDER extern)
  set_target_properties(gmock_main PROPERTIES FOLDER extern)

  macro(package_add_test TESTNAME)
    add_executable(${TESTNAME} ${ARGN})
    target_link_libraries(${TESTNAME} gtest gmock gtest_main)
    add_test(${TESTNAME} ${TESTNAME})
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
  endmacro()

  ################
  # Define a test
  package_add_test(
    test_libsettings
    src/test_request_state.cpp
    src/test_settings_util.cpp
    ${PROJECT_SOURCE_DIR}/src/request_state.c
    ${PROJECT_SOURCE_DIR}/src/settings_util.c)
  add_dependencies(test_libsettings gtest)
  include_directories(${PROJECT_SOURCE_DIR}/include)

  ################################
  # Unit Tests
  ################################
  add_custom_command(
      TARGET test_libsettings POST_BUILD
      COMMENT "Running unit tests"
      COMMAND test_libsettings
    )
endif (CMAKE_CROSSCOMPILING)
