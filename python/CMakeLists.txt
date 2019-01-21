find_package(PythonInterp 2.7 REQUIRED)

set(LIBSETTINGS_VENV "libsettings-venv")

set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY    "${PROJECT_SOURCE_DIR}/setup.py")
set(PY_OUTPUT   "${PROJECT_SOURCE_DIR}/python/libsettings.so")

if (NOT WIN32)
    set(PY_SHEBANG  "#!/usr/bin/env python")
endif (NOT WIN32)

configure_file(${SETUP_PY_IN} ${SETUP_PY})

if (WIN32)
    add_custom_command(
        OUTPUT ${LIBSETTINGS_VENV}
        COMMAND mkvirtualenv -i cython ${LIBSETTINGS_VENV}
        COMMAND add2virtualenv $ENV{WORKON_HOME}/${LIBSETTINGS_VENV}/lib/site-packages
    )

    add_custom_command(
        OUTPUT "${PY_OUTPUT}"
        COMMAND ${PYTHON} ${SETUP_PY} build_ext --force --compiler=mingw32
        COMMAND rmvirtualenv ${LIBSETTINGS_VENV}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
else (WIN32)
    add_custom_command(
        OUTPUT ${LIBSETTINGS_VENV}
        COMMAND virtualenv -p python2.7 ${LIBSETTINGS_VENV}
        COMMAND ./${LIBSETTINGS_VENV}/bin/pip install cython
    )

    add_custom_command(
        OUTPUT "${PY_OUTPUT}"
        COMMAND PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}/${LIBSETTINGS_VENV}/lib/python2.7/site-packages ${PYTHON} ${SETUP_PY} build_ext --force
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endif (WIN32)

add_custom_target(libsettings.so ALL DEPENDS ${LIBSETTINGS_VENV} ${PY_OUTPUT})
