#include "Location.h"

#include <iostream>

using namespace tok;

Location tok::operator+ (Location & lhs, Location & rhs)
{
    Location ret = lhs;
    
    if (lhs.line == rhs.line)
        ret.lastCol = rhs.lastCol;
    else //extend to end of line
        ret.setLength(ret.lineStr.length() - ret.firstCol);
    return ret;
}

void Location::setLength(int nCols)
{
    lastCol = firstCol + nCols - 1;
}

int Location::printCaret(int start)
{
    int i;
    for (i = start; i < firstCol; ++i)
        std::cerr.put(' ');
    std::cerr.put('^');

    return i + 1;
}

int Location::printPostCaret(int start)
{
    int i;
    for (i = start; i < lastCol + 1; ++i)
        std::cerr.put(' ');
    std::cerr.put('^');

    return i + 1;
}

int Location::printUnderline(int start)
{
    int i;
    for (i = start; i < firstCol; ++i)
        std::cerr.put(' ');
    for (; i <= lastCol; ++i)
        std::cerr.put('~');

    return i;
}
