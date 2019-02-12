if (CMAKE_CROSSCOMPILING)
  message(STATUS "Skipping unit tests, cross compiling")
elseif (MSVC)
  message(STATUS "Skipping unit tests, MSVC detected")
elseif (SKIP_UNIT_TESTS)
  message(STATUS "Skipping unit tests requested")
else ()
  # Download and unpack googletest at configure time
  configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
  if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
  endif()
  execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
  if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
  endif()

  # Prevent overriding the parent project's compiler/linker
  # settings on Windows
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  # Add googletest directly to our build. This defines
  # the gtest and gtest_main targets.
  add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                   ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                   EXCLUDE_FROM_ALL)

  # The gtest/gtest_main targets carry header search path
  # dependencies automatically when using CMake 2.8.11 or
  # later. Otherwise we have to add them here ourselves.
  if (CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include")
  endif()

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
    src/test_settings_util.cpp)
  add_dependencies(test_libsettings gtest)
  target_include_directories(test_libsettings PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_link_libraries(test_libsettings settings gtest_main)
  set_property(TARGET test_libsettings PROPERTY CXX_STANDARD 11)
  set_property(TARGET test_libsettings PROPERTY CXX_STANDARD_REQUIRED ON)
  set_property(TARGET test_libsettings PROPERTY CXX_EXTENSIONS OFF)



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
