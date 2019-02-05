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

### Linux Native

#### Prerequisities

* Python 2.7
* virtualenv

#### Commands

``` sh
mkdir build
cd build
cmake ..
make # output can be found in build/src/libsettings.so
# If you want to update python bindings:
cd ..
python setup.py sdist --dist-dir python
```

### Win32

#### Prerequisities

* Python 2.7 32-bit
* virtualenv (for Python 2.7 32-bit)
* virtualenvwrapper-win (for Python 2.7 32-bit)
* conan

If you have 64-bit python installed, you can install 32-bit version to arbitrary <PYTHON_PATH> directory.

#### Commands

``` sh
conan install . --profile=mingw
activate # conan
md build
cd build
set PYTHON_PATH=<PYTHON_PATH> # Needed only if default python installation is 64-bit
# example 'set PYTHON_PATH=c:\Python27_32'
cmake .. -G "MinGW Makefiles"
make
# If you want to update python bindings:
cd ..
python setup.py sdist --dist-dir python
deactivate # conan
```

## Example usage

TODO. First stage, insert link to PFWP settings client after it's merged..
