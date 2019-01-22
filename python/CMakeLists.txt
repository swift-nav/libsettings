set(LIBSETTINGS_VENV "libsettings-venv")

set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
set(SETUP_PY    "${PROJECT_SOURCE_DIR}/setup.py")
set(PY_OUTPUT   "${PROJECT_SOURCE_DIR}/python/libsettings.so")

if (NOT WIN32)
    set(PY_SHEBANG  "#!/usr/bin/env python")
endif (NOT WIN32)

configure_file(${SETUP_PY_IN} ${SETUP_PY})

if (WIN32)
    execute_process(COMMAND CMD /c python.exe -c "import sys;print(sys.exec_prefix)" OUTPUT_VARIABLE PYTHON_PATH_VENV OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND CMD /c python.exe -c "import sys;print(sys.real_prefix)" OUTPUT_VARIABLE PYTHON_PATH_REAL OUTPUT_STRIP_TRAILING_WHITESPACE)
    SET(PYTHON_SCRIPTS "${PYTHON_PATH_VENV}/Scripts")

    add_custom_command(
        OUTPUT ${LIBSETTINGS_VENV}
        COMMAND gendef - c:/windows/system32/python27.dll > ${CMAKE_BINARY_DIR}/python27.def
        COMMAND dlltool --dllname python27.dll --def ${CMAKE_BINARY_DIR}/python27.def --output-lib ${CMAKE_BINARY_DIR}/libpython27.a
    )
    file(WRITE "${PYTHON_PATH_REAL}/Lib/site-packages/virtualenv_path_extensions.pth" "$ENV{VIRTUAL_ENV}/lib/site-packages")

    add_custom_command(
        OUTPUT "${PY_OUTPUT}"
        COMMAND ${PYTHON} ${SETUP_PY} build_ext --force --compiler=mingw32 -L ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
else (WIN32)
    add_custom_command(
        OUTPUT ${LIBSETTINGS_VENV}
        COMMAND virtualenv -p python2.7 ${LIBSETTINGS_VENV}
        COMMAND ./${LIBSETTINGS_VENV}/bin/pip install -r ${PROJECT_SOURCE_DIR}/requirements-unix.txt
    )

    add_custom_command(
        OUTPUT "${PY_OUTPUT}"
        COMMAND PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}/${LIBSETTINGS_VENV}/lib/python2.7/site-packages ${PYTHON} ${SETUP_PY} build_ext --force
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endif (WIN32)

add_custom_target(libsettings.so ALL DEPENDS ${LIBSETTINGS_VENV} ${PY_OUTPUT})
