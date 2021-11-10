
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
