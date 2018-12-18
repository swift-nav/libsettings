IF(NOT ${PYTHON})
    find_program(PYTHON "python")
ENDIF()

set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY    "${PROJECT_SOURCE_DIR}/setup.py")
set(PY_OUTPUT   "${PROJECT_SOURCE_DIR}/python/libsettings.so")

configure_file(${SETUP_PY_IN} ${SETUP_PY})

add_custom_command(OUTPUT "${PY_OUTPUT}"
                   COMMAND chmod u+x ${SETUP_PY}
                   COMMAND ${PYTHON} ${SETUP_PY} build_ext --force
                   WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                  )

add_custom_target(libsettings.so ALL DEPENDS ${PY_OUTPUT})
