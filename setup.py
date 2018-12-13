#!/usr/bin/python
# Copyright (C) 2018 Swift Navigation Inc.
# Contact: <dev@swift-nav.com>
#
# This source is subject to the license found in the file 'LICENSE' which must
# be be distributed together with this source. All other rights reserved.
#
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
# EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

from glob import glob
from setuptools import setup, Extension

import os

include_dirs = ["include", "libsbp/c/include"]
sources = glob("python/*.pyx") + glob("src/*.c")

cflags = ["-Wno-unused-label", "-std=c99"]
ldflags = []

if os.name == 'nt':
    ldflags.append("-static-libgcc")

setup(
    name = 'libsettings',
    version="0.1.0",
    author="Swift Navigation",
    author_email="dev@swift-nav.com",
    description="Open source SwiftNav settings API library.",
    url = "https://github.com/swift-nav/libsettings",
    ext_modules=[
        Extension(
            'libsettings',
            sources,
            include_dirs=include_dirs,
            extra_compile_args=cflags,
            extra_link_args=ldflags,
        )
    ],
)
