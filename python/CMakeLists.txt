set(LIBSETTINGS_VENV "libsettings-venv")

set(SETUP_PY    "${PROJECT_SOURCE_DIR}/setup.py")
set(PY_OUTPUT   "libsettings.so")

# If you want to specify python version: 'cmake -D PYTHON=python3 ..'
if (NOT PYTHON)
    set(PYTHON "python")
endif (NOT PYTHON)

# In case of MSVC let the CMake do the work, no need to intervene.
if (NOT MSVC)
    if (WIN32)
        # In case of MinGW some setup is needed.
        # Figure out the Python version
        execute_process(
            COMMAND python -c "import sys;print('{}{}'.format(sys.version_info[0], sys.version_info[1]))"
            OUTPUT_VARIABLE PYTHON_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        execute_process(
            COMMAND python -c "import sys;sys.path.append('${PROJECT_SOURCE_DIR}/python/');import utils;utils.get_python_dll_path(${PYTHON_VERSION})"
            OUTPUT_VARIABLE PYDLL_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # http://www.mingw.org/wiki/createimportlibraries
        add_custom_command(
            OUTPUT ${LIBSETTINGS_VENV}
            COMMAND gendef - ${PYDLL_PATH} > ${CMAKE_BINARY_DIR}/python${PYTHON_VERSION}.def
            COMMAND dlltool --dllname python${PYTHON_VERSION}.dll --def ${CMAKE_BINARY_DIR}/python${PYTHON_VERSION}.def --output-lib ${CMAKE_BINARY_DIR}/libpython${PYTHON_VERSION}-gen.a
        )

        # Setup the virtualenv search path so that Cython is found
        execute_process(
            COMMAND CMD /c ${PYTHON} -c "import sys;print(getattr(sys, 'real_prefix', ''))"
            OUTPUT_VARIABLE PYTHON_PATH_REAL
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if (${PYTHON_PATH_REAL})
            file(WRITE "${PYTHON_PATH_REAL}/Lib/site-packages/virtualenv_path_extensions.pth" "$ENV{VIRTUAL_ENV}/lib/site-packages")
        endif ()

        # Build extension
        add_custom_command(
            DEPENDS ${LIBSETTINGS_VENV}
            OUTPUT "${PY_OUTPUT}"
            COMMAND ${PYTHON} ${SETUP_PY} build_ext --force --compiler=mingw32 -L ${CMAKE_BINARY_DIR}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        )
    else (WIN32)
        # Unix
        # Check if virtual env is available
        find_program(VIRTUALENV virtualenv)

        # Figure out the Python version
        execute_process(
            COMMAND ${PYTHON} -c "import sys;print('{}.{}'.format(sys.version_info[0], sys.version_info[1]))"
            OUTPUT_VARIABLE PYTHON_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if (VIRTUALENV)
            # In case of virtualenv use pip and paths accordingly
            add_custom_command(
                OUTPUT ${LIBSETTINGS_VENV}
                COMMAND virtualenv ${LIBSETTINGS_VENV} -p ${PYTHON}
                COMMAND ./${LIBSETTINGS_VENV}/bin/pip install -r ${PROJECT_SOURCE_DIR}/requirements-dev.txt
            )
            add_custom_command(
                DEPENDS ${LIBSETTINGS_VENV}
                OUTPUT "${PY_OUTPUT}"
                COMMAND ./${LIBSETTINGS_VENV}/bin/cythonize ${PROJECT_SOURCE_DIR}/python/libsettings.pyx
                COMMAND PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}/${LIBSETTINGS_VENV}/lib/python${PYTHON_VERSION}/site-packages ${PYTHON} ${SETUP_PY} build_ext --force
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            )
        else (VIRTUALENV)
            add_custom_command(
                OUTPUT ${LIBSETTINGS_VENV}
                COMMAND pip3 install -r ${PROJECT_SOURCE_DIR}/requirements-dev.txt
            )
            add_custom_command(
                DEPENDS ${LIBSETTINGS_VENV}
                OUTPUT "${PY_OUTPUT}"
                COMMAND cythonize ${PROJECT_SOURCE_DIR}/python/libsettings.pyx
                COMMAND ${PYTHON} ${SETUP_PY} build_ext --force
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            )
        endif (VIRTUALENV)

    endif (WIN32)
    add_custom_target(libsettings.so ALL DEPENDS ${LIBSETTINGS_VENV} ${PY_OUTPUT})
endif (NOT MSVC)
