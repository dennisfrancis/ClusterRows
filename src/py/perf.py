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
import time

class PerfTimer(object):
    def __init__(self, name, showStart=False, level=0, logger: logging.Logger=None):
        self.name = name
        self.showStart = showStart
        self.indent = '  ' * level
        self.logger = logger
        if self.showStart:
            self.print(f"{self.indent}{self.name} START")
        self.start = time.time()

    def print(self, msg: str):
        if self.logger:
            self.logger.debug(msg)
        else:
            print(msg, flush=True)

    def elapsedMSFormatted(self, prec = 2) -> str:
        s = time.time() - self.start
        return '{} ms'.format(round(s * 1000, prec))

    def show(self):
        end = " END" if self.showStart else ""
        self.print(f'{self.indent}{self.name}{end} : {self.elapsedMSFormatted()}')

    def reset(self):
        if self.showStart:
            self.print(f"{self.indent}{self.name} RESTART")
        self.start = time.time()
