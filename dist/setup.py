#!/usr/bin/env python
# Copyright (C) 2019 Swift Navigation Inc.
# Contact: <dev@swift-nav.com>
#
# This source is subject to the license found in the file 'LICENSE' which must
# be be distributed together with this source. All other rights reserved.
#
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
# EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

# This file provides index like behavior for pip install git+ under this dir
import os, sys
os.system("pip install --no-index --find-links=. libsettings")


# Rest is dummy stuff to make pip install git+... happy which wants to run
# python setup.py egg_info and other steps. Currently not known how to skip.
# Use same version number so this setup will be handled as "Requirement already
# satisfied".
from setuptools import setup
import pkg_resources

setup(
    name='libsettings',
    version=pkg_resources.get_distribution("libsettings").version,
)
