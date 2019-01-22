set(LIBSETTINGS_VENV "libsettings-venv")

set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY    "${PROJECT_SOURCE_DIR}/setup.py")
set(PY_OUTPUT   "${PROJECT_SOURCE_DIR}/python/libsettings.so")

if (NOT WIN32)
    set(PY_SHEBANG  "#!/usr/bin/env python")
endif (NOT WIN32)

if (NOT PYTHON)
    set(PYTHON "python")
endif (NOT PYTHON)

if (WIN32)
    execute_process(COMMAND CMD /c python.exe -c "import sys;print(sys.exec_prefix)" OUTPUT_VARIABLE PYTHON_PATH_VENV OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND CMD /c python.exe -c "import sys;print(sys.real_prefix)" OUTPUT_VARIABLE PYTHON_PATH_REAL OUTPUT_STRIP_TRAILING_WHITESPACE)
    SET(PYTHON_SCRIPTS "${PYTHON_PATH_VENV}/Scripts")

    execute_process(
        COMMAND python -c "import sys;print('{}{}'.format(sys.version_info[0], sys.version_info[1]))"
        OUTPUT_VARIABLE PYTHON_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(PYTHON_GENERATED_LIB "python${PYTHON_VERSION}-gen")

    configure_file(${SETUP_PY_IN} ${SETUP_PY})

    add_custom_command(
        OUTPUT ${LIBSETTINGS_VENV}
        COMMAND gendef - c:/windows/system32/python${PYTHON_VERSION}.dll > ${CMAKE_BINARY_DIR}/python${PYTHON_VERSION}.def
        COMMAND dlltool --dllname python${PYTHON_VERSION}.dll --def ${CMAKE_BINARY_DIR}/python${PYTHON_VERSION}.def --output-lib ${CMAKE_BINARY_DIR}/lib${PYTHON_GENERATED_LIB}.a
    )
    file(WRITE "${PYTHON_PATH_REAL}/Lib/site-packages/virtualenv_path_extensions.pth" "$ENV{VIRTUAL_ENV}/lib/site-packages")

    add_custom_command(
        OUTPUT "${PY_OUTPUT}"
        COMMAND python.exe ${SETUP_PY} build_ext --force --compiler=mingw32 -L ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
else (WIN32)

    add_custom_command(
        OUTPUT ${LIBSETTINGS_VENV}
        COMMAND virtualenv ${LIBSETTINGS_VENV} -p ${PYTHON}
        COMMAND ./${LIBSETTINGS_VENV}/bin/pip install -r ${PROJECT_SOURCE_DIR}/requirements-unix.txt
    )

    execute_process(
        COMMAND ${PYTHON} -c "import sys;print('{}.{}'.format(sys.version_info[0], sys.version_info[1]))"
        OUTPUT_VARIABLE PYTHON_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    configure_file(${SETUP_PY_IN} ${SETUP_PY})

    add_custom_command(
        OUTPUT "${PY_OUTPUT}"
        COMMAND PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}/${LIBSETTINGS_VENV}/lib/python${PYTHON_VERSION}/site-packages ${PYTHON} ${SETUP_PY} build_ext --force
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endif (WIN32)

add_custom_target(libsettings.so ALL DEPENDS ${LIBSETTINGS_VENV} ${PY_OUTPUT})
