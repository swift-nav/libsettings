set(libsettings_HEADERS
  ${PROJECT_SOURCE_DIR}/include/libsettings/settings.h
  ${PROJECT_SOURCE_DIR}/include/libsettings/settings_declspec.h
  ${PROJECT_SOURCE_DIR}/include/libsettings/settings_util.h
  )

add_library(settings
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

target_link_libraries(settings libswiftnav::swiftnav libsbp::sbp)

target_include_directories(settings PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

swift_install_targets(
    TARGETS settings
    HEADER_DIRS ${PROJECT_SOURCE_DIR}/include/libsettings
    )
