find_package(PythonInterp 2.7 REQUIRED)

set(VIRTUALENV virtualenv)
set(VIRTUALENV ${VIRTUALENV} -p python2.7)
set(LIBSETTINGS_VENV "venv")

set(VIRTUALENV ${VIRTUALENV})
set(LIBSETTINGS_VENV "libsettings-venv")

add_custom_command(
    OUTPUT ${LIBSETTINGS_VENV}
    COMMAND mkvirtualenv -i cython ${LIBSETTINGS_VENV}
    COMMAND add2virtualenv $ENV{WORKON_HOME}/${LIBSETTINGS_VENV}/lib/site-packages
)

set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY    "${PROJECT_SOURCE_DIR}/setup.py")
set(PY_OUTPUT   "${PROJECT_SOURCE_DIR}/python/libsettings.so")

configure_file(${SETUP_PY_IN} ${SETUP_PY})

add_custom_command(OUTPUT "${PY_OUTPUT}"
                   COMMAND ${PYTHON} ${SETUP_PY} build_ext --force --compiler=mingw32
                   COMMAND rmvirtualenv ${LIBSETTINGS_VENV}
                   WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                  )

add_custom_target(libsettings.so ALL DEPENDS ${LIBSETTINGS_VENV} ${PY_OUTPUT})
