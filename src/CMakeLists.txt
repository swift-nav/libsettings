set(libsettings_HEADERS
  ${PROJECT_SOURCE_DIR}/include/libsettings/settings.h
  ${PROJECT_SOURCE_DIR}/include/libsettings/settings_util.h
  )

add_library(settings settings.c settings_util.c request_state.c setting_type.c)
target_link_libraries(settings sbp)

target_include_directories(settings PUBLIC ${PROJECT_SOURCE_DIR}/include)

install(TARGETS settings DESTINATION lib)
install(FILES ${libsettings_HEADERS} DESTINATION include/libsettings)
