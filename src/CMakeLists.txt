################################################################################
# Build Targets
################################################################################
set(libsettings_HEADERS
  ${PROJECT_SOURCE_DIR}/include/libsettings/settings.h
  ${PROJECT_SOURCE_DIR}/include/libsettings/settings_util.h)

add_library(settings
            ${LIBSWIFTNAV_PATH}/src/logging.c
            ${LIBSWIFTNAV_PATH}/src/logging_common.c
            settings.c
            settings_util.c
            request_state.c
            setting_data.c
            setting_sbp_cb.c
            setting_type.c
            setting_type_enum.c
            setting_type_float.c
            setting_type_int.c
            setting_type_str.c)

target_include_directories(settings PUBLIC ${PROJECT_SOURCE_DIR}/include)
set_property(TARGET settings PROPERTY C_STANDARD 99)
set_property(TARGET settings PROPERTY C_STANDARD_REQUIRED ON)
set_property(TARGET settings PROPERTY C_EXTENSIONS OFF)

if(EXISTS ${LIBSBP_PATH}/c/include/)
    target_include_directories(settings PUBLIC ${LIBSBP_PATH}/c/include/)
else()
    message(WARNING "No LibSbp include path found.")
endif()

if (EXISTS ${LIBSWIFTNAV_PATH}/include/)
    target_include_directories(settings PRIVATE ${LIBSWIFTNAV_PATH}/include)
else()
    message(WARNING "No LibSwiftNav include path found.")
endif()

if (NOT MSVC)
    target_compile_options(settings PRIVATE
            -Wall -Wextra -Wno-strict-prototypes -Werror -fno-unwind-tables
            -fno-asynchronous-unwind-tables -Wimplicit -Wshadow -Wswitch-default
            -Wswitch-enum -Wundef -Wuninitialized -Wcast-align -Wformat=2
            -Wimplicit-function-declaration -Wredundant-decls -Wformat-security
            -ggdb)
endif (NOT MSVC)

if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(settings PRIVATE -Wno-unknown-warning-option -Wno-error=typedef-redefinition)
endif()

if (ENABLE_STACK_ANALYSIS AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message(STATUS "Enabling stack analysis.")
    target_compile_options(settings PUBLIC -fstack-usage -fstack-check)
endif ()

################################################################################
# Installation
################################################################################
# Export these targets to a cmake file in the build directory, useful for projects that
# include libsettings as source
export(TARGETS settings
        NAMESPACE LibSettings::
        FILE ${PROJECT_BINARY_DIR}/LibSettingsImport.cmake)
