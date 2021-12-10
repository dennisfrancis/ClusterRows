# ClusterRows
# Copyright (c) 2021 Dennis Francis <dennisfrancis.in@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import logging
from pathlib import Path, PurePath

_logPath = '.'
_logger = None

def setupLogger(logPath: str) -> logging.Logger:
    global _logPath
    global _logger
    if not (_logger is None): return _logger

    _logPath = logPath
    Path(_logPath).mkdir(parents=True, exist_ok=True)
    _logger = logging.getLogger()
    _logger.setLevel(logging.DEBUG)
    logfile = str(PurePath(_logPath, "log.txt"))
    fh = logging.FileHandler(logfile, mode='a', encoding=None, delay=False)
    fh.setLevel(logging.DEBUG)
    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
    fh.setFormatter(formatter)
    _logger.addHandler(fh)
    _logger.debug("LOGGING INIT")
    return _logger

def getLogger() -> logging.Logger:
    return _logger
