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

* Python 2.7
* cmake
* virtualenv (pip install virtualenv)

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

### Windows

#### NOTE

This process will overwrite <PYTHON_PATH>/Lib/site-packages/virtualenv_path_extensions.pth
This is a file used by virtualenv and doesn't belong to standard python installation.
In case you are using virtualenv with other projects you might want to check if this file
exists in your setup and make a copy of it.

#### Prerequisities

* Python 2.7
* virtualenvwrapper-win (pip install virtualenvwrapper-win)

#### Commands

``` sh
mkvirtualenv -r requirements-win.txt venv
md build
cd build
cmake .. -G "MinGW Makefiles"
make
# If you want to update python bindings:
cd ..
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
