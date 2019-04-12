if (CMAKE_CROSSCOMPILING)
  message(STATUS "Skipping unit tests, cross compiling")
elseif (MSVC)
  message(STATUS "Skipping unit tests, MSVC detected")
elseif (SKIP_UNIT_TESTS)
  message(STATUS "Skipping unit tests requested")
else ()

  # Now simply link against gtest or gtest_main as needed. Eg
  add_executable(
    test_libsettings
    src/test_request_state.cpp
    src/test_setting_data.cpp
    src/test_setting_sbp_cb.cpp
    src/test_setting_type_enum.cpp
    src/test_setting_type_float.cpp
    src/test_setting_type_int.cpp
    src/test_setting_type_str.cpp
    src/test_settings_util.cpp
    src/test_stubs.cpp
    ${PROJECT_SOURCE_DIR}/src/request_state.c
    ${PROJECT_SOURCE_DIR}/src/setting_data.c
    ${PROJECT_SOURCE_DIR}/src/setting_sbp_cb.c
    ${PROJECT_SOURCE_DIR}/src/setting_type.c
    ${PROJECT_SOURCE_DIR}/src/setting_type_enum.c
    ${PROJECT_SOURCE_DIR}/src/setting_type_float.c
    ${PROJECT_SOURCE_DIR}/src/setting_type_int.c
    ${PROJECT_SOURCE_DIR}/src/setting_type_str.c
    ${PROJECT_SOURCE_DIR}/src/settings.c
    ${PROJECT_SOURCE_DIR}/src/settings_util.c)
  include_directories(${PROJECT_SOURCE_DIR}/include)
  include_directories(${PROJECT_SOURCE_DIR}/tests/include)
  target_link_libraries(test_libsettings GTest::main libswiftnav::swiftnav)

  if (WIN32)
    add_custom_command(
      TARGET test_libsettings POST_BUILD
      COMMENT "Running unit tests"
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bin/libgtest.dll ${CMAKE_BINARY_DIR}/tests
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bin/libgtest_main.dll ${CMAKE_BINARY_DIR}/tests
      COMMAND test_libsettings
    )
  else (WIN32)
    add_custom_command(
      TARGET test_libsettings POST_BUILD
      COMMENT "Running unit tests"
      COMMAND test_libsettings
    )
  endif (WIN32)

endif ()
