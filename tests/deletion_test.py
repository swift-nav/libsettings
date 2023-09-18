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

_LOGGER = logging.getLogger(__name__)


class SourceIterableStub:
    def __iter__(self):
        return self

    def __next__(self):
        raise StopIteration

    def breakiter(self):
        pass


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)

    with Handler(SourceIterableStub()) as link:
        _LOGGER.debug('Expecting message "Building settings framework" in STDOUT...')
        settings = Settings(link)

    _LOGGER.debug('Expecting message "Releasing settings framework" in STDOUT...')
    # Settings object destruction should happen around here when Settings goes of out scope and garbage collection
    # is triggered
