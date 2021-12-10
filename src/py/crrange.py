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

def cellAddressToString(col, row) -> str:
    buf = bytearray('', encoding="utf-8")
    aVal = ord('A')
    dollarVal = ord('$')
    if col < 26 * 26:
        buf.append(dollarVal)
        if col < 26:
            buf.append(aVal + col)
        else:
            buf.append(aVal + (col // 26) - 1)
            buf.append(aVal + (col % 26))
    else:
        while col >= 26:
            rem = col % 26
            buf.append(aVal + rem)
            col = col - rem
            col = (col // 26) - 1
        buf.append(aVal + col)
        buf.append(dollarVal)
        buf.reverse()

    buf.append(dollarVal)
    buf.extend(b'%d' % (row + 1))
    return buf.decode()

def sheetName(sheetNum, document):
    return document.Sheets[sheetNum].Name

def cellRangeToString(cellRange, document) -> str:
    sName = sheetName(cellRange.Sheet, document)
    parts = [
        "$",
        sName,
        ".",
        cellAddressToString(cellRange.StartColumn, cellRange.StartRow),
        ":",
        cellAddressToString(cellRange.EndColumn, cellRange.EndRow),
    ]
    return ''.join(parts)
