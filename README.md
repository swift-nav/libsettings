Copyright (C) 2018-2019 Swift Navigation Inc.
Contact: Swift Navigation <dev@swiftnav.com>

This source is subject to the license found in the file 'LICENSE' which must
be be distributed together with this source. All other rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

# libsettings

Open source SwiftNav settings API library.

`libsettings` aims to provide standardized settings framework for projects accessing Piksi settings.

## Building

### Unix

#### Prerequisities

* Python
* CMake
* virtualenv (pip install virtualenv)

You can do without 'virtualenv' but beaware that in this case contents of
requirements-unix.txt shall be installed to your Python environment.

#### Commands

``` sh
mkdir build
cd build
cmake .. # If you want to use python3: 'cmake -D PYTHON=python3 ..'
make
cd ..
# If you want to update python bindings source distribution package:
python setup.py sdist --dist-dir python
```

### Windows

#### NOTE

In case of virtualenv this process will overwrite <PYTHON_PATH>/Lib/site-packages/virtualenv_path_extensions.pth
This is a file used by virtualenv and doesn't belong to standard python installation.
In case you are using virtualenv with other projects you might want to check if this file
exists in your setup and make a copy of it.

#### Prerequisities for Python 2.7.x or 3.4.x

* virtualenvwrapper-win (pip install virtualenvwrapper-win)

You can do without 'virtualenvwrapper-win' but beaware that in this case
contents of requirements-*.txt should be installed to your Python environment.

#### Commands for Python 2.7.x or 3.4.x

``` sh
mkvirtualenv -r requirements-win-cp27-cp34.txt venv
md build
cd build
# Make sure CMake is using venv/Scripts/gcc.exe as compiler and not some other
# gcc possibly installed
cmake .. -G "MinGW Makefiles"
make
cd ..
# If you want to update python bindings source distribution package:
python setup.py sdist --dist-dir python
deactivate # virtualenv
rmvirtualenv venv
```

#### Prerequisities for Python 3.5.x or 3.6.x or 3.7.x

* virtualenvwrapper-win (pip install virtualenvwrapper-win)
* Microsoft Visual C++ 14.0, for example from:
  https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2017

#### Commands for Python 3.5.x or 3.6.x or 3.7.x

Use 'Native Tools Command Prompt for VS 2017', select x86/x64 based on your Python architecture

``` sh
mkvirtualenv -r requirements-win-cp35-cp36-cp37.txt venv
md build
cd build
cmake ..
# settings.dll from C sources (not tested in runtime)
msbuild libsettings.sln /p:Configuration="Release" /p:Platform="Win32"
cd ..
# libsettings.pyd for importing from Python
python setup.py build_ext --force
# If you want to update python bindings source distribution package:
python setup.py sdist --dist-dir python
deactivate # virtualenv
rmvirtualenv venv
```

#### Sanity check

To test your build you should search for libsettings.pyd under `build/` directory
and copy it to `python/` directory containing settins_client.py. And then run in `python/`:

``` sh
python settings_client.py --tcp -p <piksi_ip>:55555
```

## Example usage

TODO. First stage, insert link to PFWP settings client after it's merged..
