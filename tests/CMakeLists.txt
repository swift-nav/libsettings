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

    target_compile_definitions(test_libsettings PRIVATE "_WINDLL")

  target_include_directories(test_libsettings
      PRIVATE
      ${PROJECT_SOURCE_DIR}/include
      ${PROJECT_SOURCE_DIR}/tests/include
      $<TARGET_PROPERTY:sbp,INCLUDE_DIRECTORIES>
      )

  target_link_libraries(test_libsettings swiftnav gtest_main)

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

