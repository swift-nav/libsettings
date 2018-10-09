set(libsettings_HEADERS
  ${PROJECT_SOURCE_DIR}/include/libsettings/settings_register.h
  )

add_library(settings settings_register.c)
target_link_libraries(settings sbp)

target_include_directories(settings PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(settings PUBLIC ${PROJECT_SOURCE_DIR}/src)

install(TARGETS settings DESTINATION lib)
install(FILES ${libsettings_HEADERS} DESTINATION include/libsettings)