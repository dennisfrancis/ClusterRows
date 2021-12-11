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

def cellAddressToString(col, row, absolute = True) -> str:
    buf = bytearray('', encoding="utf-8")
    aVal = ord('A')
    dollarVal = ord('$')
    if col < 26 * 26:
        if absolute:
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
        if absolute:
            buf.append(dollarVal)
        buf.reverse()

    if absolute:
        buf.append(dollarVal)
    buf.extend(b'%d' % (row + 1))
    return buf.decode()

def sheetName(sheetNum, document):
    return document.Sheets[sheetNum].Name

def cellRangeToString(cellRange, document) -> str:
    if cellRange is None:
        return ''
    sName = sheetName(cellRange.Sheet, document)
    parts = [
        "$'",
        sName,
        "'.",
        cellAddressToString(cellRange.StartColumn, cellRange.StartRow),
        ":",
        cellAddressToString(cellRange.EndColumn, cellRange.EndRow),
    ]
    return ''.join(parts)

def stringToRangeObj(rangeStr: str, document):
    dotPos = rangeStr.find('.')
    sheet = None
    rangeStrSheet = rangeStr
    if dotPos != -1:
        sheetName = rangeStr[:dotPos].strip("$'")
        try:
            sheet = document.Sheets.getByName(sheetName)
        except Exception as e:
            sheet = None
        rangeStrSheet = rangeStr[dotPos + 1:]
    else:
        sheet = document.getCurrentController().getActiveSheet()

    if sheet is None:
        return None

    try:
        rangeObj = sheet.getCellRangeByName(rangeStrSheet)
    except Exception as e:
        rangeObj = None
    return rangeObj

def rangeAddressToObject(rangeAddr, document):
    if rangeAddr is None:
        return None

    sheet = document.Sheets[rangeAddr.Sheet]
    try:
        rangeObj = sheet.getCellRangeByPosition(rangeAddr.StartColumn, rangeAddr.StartRow, rangeAddr.EndColumn, rangeAddr.EndRow)
    except Exception as e:
        rangeObj = None
    return rangeObj

def stringToCellRange(rangeStr: str, document):
    rangeObj = stringToRangeObj(rangeStr, document)
    return None if rangeObj is None else rangeObj.getRangeAddress()

def isStringRangeValid(rangeStr: str, document):
    rangeObj = stringToRangeObj(rangeStr, document)
    return False if rangeObj is None else True
