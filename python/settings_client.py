#!/usr/bin/env python
# Copyright (C) 2018 Swift Navigation Inc.
# Contact: Swift Navigation <dev@swiftnav.com>
#
# This source is subject to the license found in the file 'LICENSE' which must
# be be distributed together with this source. All other rights reserved.
#
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
# EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

# This script resets the FDTI settings to its default values

from __future__ import absolute_import, print_function

import time

from sbp.client import Framer, Handler
from sbp.client.drivers.network_drivers import TCPDriver
from sbp.settings import SBP_MSG_SETTINGS_SAVE, SBP_MSG_SETTINGS_WRITE

from piksi_tools import serial_link

from settings import Settings


def send_setting(link, section, name, value):
    link.send(SBP_MSG_SETTINGS_WRITE, '%s\0%s\0%s\0' % (section, name, value))


def get_args():
    """
    Get and parse arguments.
    """
    import argparse
    parser = argparse.ArgumentParser(description='libsettings example')
    parser.add_argument(
        '-p',
        '--port',
        default=[serial_link.SERIAL_PORT],
        nargs=1,
        help='specify the serial port to use.')
    parser.add_argument(
        "-b",
        "--baud",
        default=[serial_link.SERIAL_BAUD],
        nargs=1,
        help="specify the baud rate to use.")
    parser.add_argument(
        "-f",
        "--ftdi",
        help="use pylibftdi instead of pyserial.",
        action="store_true")
    parser.add_argument(
        "--tcp",
        action="store_true",
        default=False,
        help="Use a TCP connection instead of a local serial port. \
                      If TCP is selected, the port is interpreted as host:port"
    )
    return parser.parse_args()


def main():
    """
    Get configuration, get driver, and build handler and start it.
    """
    args = get_args()
    port = args.port[0]
    baud = args.baud[0]

    if args.tcp:
        try:
            host, port = port.split(':')
            selected_driver = TCPDriver(host, int(port))
        except:  # noqa
            raise Exception('Invalid host and/or port')
    else:
        selected_driver = serial_link.get_driver(args.ftdi, port, baud)

    # Driver with context
    with selected_driver as driver:
        # Handler with context
        with Handler(Framer(driver.read, driver.write)) as link:
            print("Creating Settings")
            s = Settings(1234, link)

            #print("Registering libsettings.test")
            #s.register("libsettings", "test", None, 0, None)

            time.sleep(1)

            #print("frontend.antenna_selection =", s.read("frontend", "antenna_selection"))
            #value = raw_input('Enter new frontend.antenna_selection value: ')
            #s.write("frontend", "antenna_selection", value)
            #print("frontend.antenna_selection =", s.read("frontend", "antenna_selection"))

            #print("solution.elevation_mask =", s.read("solution", "elevation_mask"))
            #value = raw_input('Enter new solution.elevation_mask value: ')
            value = 10
            raw_input('Press enter to read all settings..')
            s.write("solution", "elevation_mask", value)
            print("solution.elevation_mask =", s.read("solution", "elevation_mask"))

            #raw_input('Press enter to read all settings..')

            l = s.read_all()

            for setting in l:
                print(setting)

            print("Release Settings")
            s.destroy()


if __name__ == "__main__":
    main()
