set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(include_install_dir "${CMAKE_INSTALL_INCLUDEDIR}")
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets.cmake")
set(namespace "${PROJECT_NAME}::")

set(settings_SRCS
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

add_library(settings ${settings_SRCS})
add_library(${namespace}settings ALIAS settings)

target_link_libraries(settings libswiftnav::swiftnav)

target_include_directories(settings 
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    )

include(CMakePackageConfigHelpers)

write_basic_package_version_file("${version_config}" COMPATIBILITY SameMajorVersion)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/Config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}"
    )

install(
    TARGETS settings
    EXPORT "${TARGETS_EXPORT_NAME}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${include_install_dir}"
    )

install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/libsettings"
    DESTINATION "${include_install_dir}"
    )

install(
    FILES "${project_config}" "${version_config}"
    DESTINATION "${config_install_dir}"
    )

install(
    EXPORT "${TARGETS_EXPORT_NAME}"
    NAMESPACE "${namespace}"
    DESTINATION "${config_install_dir}"
    )
