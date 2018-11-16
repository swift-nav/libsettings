set(libsettings_HEADERS
  ${PROJECT_SOURCE_DIR}/include/libsettings/settings.h
  ${PROJECT_SOURCE_DIR}/include/internal/registration_state.h
  )

add_library(settings settings.c registration_state.c)
target_link_libraries(settings sbp)

target_include_directories(settings PUBLIC ${PROJECT_SOURCE_DIR}/include)

install(TARGETS settings DESTINATION lib)
install(FILES ${libsettings_HEADERS} DESTINATION include/libsettings)