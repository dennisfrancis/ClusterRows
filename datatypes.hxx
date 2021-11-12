/*
 * ClusterRows
 * Copyright (c) 2021 Dennis Francis <dennisfrancis.in@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <sal/types.h>
#include <rtl/ustring.hxx>
#include <rtl/ustrbuf.hxx>

using rtl::OUString;
using rtl::OUStringBuffer;
using rtl::OUStringHash;

enum class DataType
{
    INTEGER,
    DOUBLE,
    STRING
};

static const char* DataType2String(DataType eType)
{
    switch (eType)
    {
        case DataType::INTEGER:
            return "INTEGER";
        case DataType::DOUBLE:
            return "DOUBLE";
        case DataType::STRING:
            return "STRING";
    }

    return "UNKNOWN";
}

struct ColorsRGB
{
    double Red;
    double Green;
    double Blue;
};
