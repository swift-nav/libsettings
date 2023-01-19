# Copyright (C) 2018 Swift Navigation Inc.
# Contact: <dev@swift-nav.com>
#
# This source is subject to the license found in the file 'LICENSE' which must
# be be distributed together with this source. All other rights reserved.
#
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
# EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

import os.path

from glob import glob
from setuptools import setup, Extension

import os
import sys

CLASSIFIERS = [
    'Intended Audience :: Developers',
    'Intended Audience :: Science/Research',
    'Operating System :: POSIX :: Linux',
    'Operating System :: MacOS :: MacOS X',
    'Operating System :: Microsoft :: Windows',
    'Programming Language :: Python',
    'Topic :: Scientific/Engineering :: Interface Engine/Protocol Translator',
    'Topic :: Software Development :: Libraries :: Python Modules',
    'Programming Language :: Python :: 3.6',
    'Programming Language :: Python :: 3.7',
    'Programming Language :: Python :: 3.8',
    'Programming Language :: Python :: 3.9',
    'Programming Language :: Python :: 3.10',
    'Programming Language :: Python :: 3.11',
]

PLATFORMS = [
    'linux',
    'osx',
    'win32',
]

HERE = os.path.dirname(__file__)
if HERE:
    os.chdir(HERE)
include_dirs = ["include", "third_party/libsbp/c/include",
                "third_party/libswiftnav/include"]
sources = glob("python/*.pyx") + glob("src/*.c") + \
    glob("third_party/libswiftnav/src/logging*.c")
py_version = '{}{}'.format(sys.version_info[0], sys.version_info[1])

cflags = ["-Wno-unused-label", "-std=c99"]
ldflags = []
libraries = []

if os.name == 'nt':
    if py_version == '27' or py_version == '34':
        ldflags.append("-static-libgcc")
        libraries.append("python{}-gen".format(py_version))
    else:
        cflags = []

setup(
    name='libsettings',
    version="0.1.12",
    author="Swift Navigation",
    author_email="dev@swift-nav.com",
    description="Open source SwiftNav settings API library.",
    url="https://github.com/swift-nav/libsettings",
    classifiers=CLASSIFIERS,
    platforms=PLATFORMS,
    ext_modules=[
        Extension(
            'libsettings',
            sources,
            libraries=libraries,
            include_dirs=include_dirs,
            extra_compile_args=cflags,
            extra_link_args=ldflags,
        )
    ],
    install_requires=["sbp"]
)
