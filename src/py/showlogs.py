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
from pathlib import Path

def main() -> None:
    home = os.path.expanduser("~")
    pyextdir = Path(home) / ".config/libreoffice/4/user/uno_packages/cache/uno_packages"
    exts = (extensions for extensions in pyextdir.iterdir() if extensions.is_dir())
    for ext in exts:
        roots = [ x for x in ext.iterdir() if x.is_dir() and x.suffix == ".oxt" and x.stem.startswith("ClusterRows-") ]
        if len(roots) > 0:
            log = roots[0] / "log.txt"
            if (not log.exists()) or (not log.is_file()):
                print("Seems ClusterRows extension was never used after install!")
                return
            print("logfile is {}".format(log))
            with open(log, "rb") as f:
                shutil.copyfileobj(f, sys.stdout.buffer)
            return
    print("Cannot find ClusterRows extension installation directory!")

if __name__ == "__main__":
    main()
