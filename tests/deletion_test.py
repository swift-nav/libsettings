"""
Manual ``__del__()`` Test

The ``libsettings`` tool is known to print two messages to STDOUT:

1. "Building settings framework" upon instantiation of the Settings object.
2. "Releasing settings framework" upon destruction of the Settings object.

When running this simple script, we expect both of the above two messages to get printed out at the appropriate time.
"""

import logging

from libsettings import Settings

from sbp.client import Framer, Handler
from sbp.client.drivers.network_drivers import TCPDriver

_PIKSI_DEFAULT_PORT = 55555
_PIKSI_DEFAULT_HOST = "192.168.0.222"

_LOGGER = logging.getLogger(__name__)


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)

    driver = TCPDriver(_PIKSI_DEFAULT_HOST, _PIKSI_DEFAULT_PORT)

    with Handler(Framer(driver.read, driver.write)) as link:
        _LOGGER.debug('Expecting message "Building settings framework" in STDOUT...')
        settings = Settings(link)

    _LOGGER.debug('Expecting message "Releasing settings framework" in STDOUT...')
    # Settings object destruction should happen around here when Settings goes of out scope and garbage collection
    # is triggered
