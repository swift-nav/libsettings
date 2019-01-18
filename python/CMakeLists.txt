IF(NOT ${PYTHON})
    find_program(PYTHON "python")
ENDIF()

find_program(VIRTUALENV virtualenv)
if(NOT VIRTUALENV)
    message(FATAL_ERROR "`virtualenv` not found")
endif()

set(VIRTUALENV ${VIRTUALENV} -p python2.7)
set(LIBSETTINGS_VENV "venv")

add_custom_command(
    OUTPUT ${LIBSETTINGS_VENV}
    COMMAND ${VIRTUALENV} ${LIBSETTINGS_VENV}
)

add_custom_command(
    OUTPUT ${LIBSETTINGS_VENV}.stamp
    DEPENDS ${LIBSETTINGS_VENV}
    COMMAND ./${LIBSETTINGS_VENV}/bin/pip install cython
)

set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY    "${PROJECT_SOURCE_DIR}/setup.py")
set(PY_OUTPUT   "${PROJECT_SOURCE_DIR}/python/libsettings.so")

configure_file(${SETUP_PY_IN} ${SETUP_PY})

add_custom_command(OUTPUT "${PY_OUTPUT}"
                   COMMAND chmod u+x ${SETUP_PY}
                   COMMAND PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}/${LIBSETTINGS_VENV}/lib/python2.7/site-packages ${PYTHON} ${SETUP_PY} build_ext --force
                   WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                  )

add_custom_target(libsettings.so ALL DEPENDS ${LIBSETTINGS_VENV}.stamp ${PY_OUTPUT})
