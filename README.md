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

#### Output

* .so can be found under `build/src/`
* Python distribution package can be found under `dist/`

#### Prerequisities

* Python
* pip
* CMake
* virtualenv (pip install virtualenv)

You can do without 'virtualenv' but beaware that in this case contents of
requirements-unix.txt shall be installed to your Python environment. You can
specify the Python version while calling 'cmake' otherwise the default one
is used.

#### Commands

``` sh
./scripts/sdist-unix.sh
```

or more explicit:

``` sh
mkdir build
cd build
cmake .. # If you want to speficy python version: 'cmake -D PYTHON=python3 ..'
make
cd ..
```

### Windows

Architecture (32/64-bit) is determined by conda installation.

#### Output

* .dll and .lib can be found under `build/src/Release/`
* Python distribution package can be found under `dist/`

#### Prerequisities for Python 2.7.x

* conda

#### Commands for Python 2.7.x

For MinGW make to work correctly sh.exe must NOT be in your path.

``` sh
./scripts/bdist-wheel-win-gcc.bat 2.7
```

#### Prerequisities for Python 3.5.x or 3.6.x or 3.7.x

* conda
* Microsoft Visual C++ 14.0, for example from:
  https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2017

#### Commands for Python 3.5.x or 3.6.x or 3.7.x

``` sh
./scripts/bdist-wheel-win-msvc.bat 3.5
./scripts/bdist-wheel-win-msvc.bat 3.6
./scripts/bdist-wheel-win-msvc.bat 3.7
```

## Sanity check

To test your build you should search for the built distribution package under
dist/ directory. Install it using ´pip´ and then:

``` sh
python settings_client.py --tcp -p <piksi_ip>:55555
```

## Installing from package managers
Some bindings are available on package managers:

* [`python`]: available on PyPI: `pip install libsettings`

### Installing development Python versions

To install the Python binding from GitHub repo (using pip) run the following command:

```sh
pip install git+https://github.com/swift-nav/libsettings.git@branch_name#subdirectory=dist
```

## Example usage

TODO. First stage, insert link to PFWP settings client after it's merged..
