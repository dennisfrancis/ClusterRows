
#ifndef __CLUSTERROWS_DATATYPES__
#define __CLUSTERROWS_DATATYPES__

#include <rtl/ustring.hxx>
#include <rtl/ustrbuf.hxx>

using rtl::OUString;
using rtl::OUStringBuffer;
using rtl::OUStringHash;

enum DataType{ INTEGER, DOUBLE, STRING };

struct ColorsRGB
{
    double Red;
    double Green;
    double Blue;
};

#endif
