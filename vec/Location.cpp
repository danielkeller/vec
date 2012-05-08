#include "Location.h"

using namespace tok;

Location operator+ (Location & lhs, Location & rhs)
{
    Location ret = lhs;
    ret.lastCol = rhs.lastCol;
    return ret;
}

void Location::setLength(int nCols)
{
    lastCol = firstCol + nCols - 1;
}