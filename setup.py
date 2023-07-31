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

import os, sys

CLASSIFIERS = [
  "Intended Audience :: Developers",
  "Intended Audience :: Science/Research",
  "Operating System :: POSIX :: Linux",
  "Operating System :: MacOS :: MacOS X",
  "Operating System :: Microsoft :: Windows",
  "Programming Language :: Python",
  "Topic :: Scientific/Engineering :: Interface Engine/Protocol Translator",
  "Topic :: Software Development :: Libraries :: Python Modules",
  "Programming Language :: Python :: 3.7",
  "Programming Language :: Python :: 3.8",
  "Programming Language :: Python :: 3.9",
  "Programming Language :: Python :: 3.10",
  "Programming Language :: Python :: 3.11",
]

PLATFORMS = [
  "linux",
  "osx",
  "win32",
]

HERE = os.path.dirname(__file__)

if HERE:
    os.chdir(HERE)

include_dirs = ["include", "third_party/libsbp/c/include", "third_party/libswiftnav/include"]
sources = glob("python/*.pyx") + glob("src/*.c") + glob("third_party/libswiftnav/src/logging*.c")

ldflags = []
libraries = []

if os.name == "nt":
    cflags = []
else:
    cflags = ["-Wno-unused-label", "-std=c99"]

setup(
    name="libsettings",
    version="0.1.14",
    author="Swift Navigation",
    author_email="dev@swift-nav.com",
    description="Open source SwiftNav settings API library.",
    url="https://github.com/swift-nav/libsettings",
    classifiers=CLASSIFIERS,
    platforms=PLATFORMS,
    options={"bdist_wheel": {"universal": True}},
    ext_modules=[
        Extension(
            "libsettings",
            sources,
            libraries=libraries,
            include_dirs=include_dirs,
            extra_compile_args=cflags,
            extra_link_args=ldflags,
        )
    ],
)
