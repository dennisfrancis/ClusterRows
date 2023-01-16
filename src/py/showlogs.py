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

import os
import shutil
import sys

def main():
    home = os.path.expanduser("~")
    pyextdir = home + "/.config/libreoffice/4/user/uno_packages/cache/uno_packages"
    exts = list(filter(os.path.isdir, (os.path.join(pyextdir, x) for x in os.listdir(pyextdir))))
    for ext in exts:
        roots = [ x for x in os.listdir(ext) if x.endswith(".oxt") and x.startswith("ClusterRows-") ]
        if len(roots) > 0:
            log = os.path.join(ext, roots[0], "log.txt")
            if not os.path.isfile(log):
                print("Seems ClusterRows extension was never used after install!")
                return
            print("logfile is " + log)
            with open(log, "rb") as f:
                shutil.copyfileobj(f, sys.stdout.buffer)
            return
    print("Cannot find ClusterRows extension installation directory!")
    print(exts)

if __name__ == '__main__':
    main()
