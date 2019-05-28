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

target_link_libraries(settings swiftnav)

target_include_directories(settings PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(settings PRIVATE $<TARGET_PROPERTY:sbp,INCLUDE_DIRECTORIES>)

install(TARGETS settings DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
install(FILES ${libsettings_HEADERS} DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/libsettings)
