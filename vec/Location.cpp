#include "Location.h"

#include <iostream>

using namespace tok;

Location::Location()
    : line(0),
    firstCol(0),
    lastCol(0),
    fileName(""),
    lineStr(fileName, fileName) //beginning and ending of empty str.
{
}

Location tok::operator+ (Location & lhs, Location & rhs)
{
    Location ret = lhs;
    
    if (lhs.line == rhs.line)
        ret.lastCol = rhs.lastCol;
    else //extend to end of line
        ret.setLength(ret.lineStr.length() - ret.firstCol);
    return ret;
}

Location& tok::operator+= (Location & lhs, Location & rhs)
{
    if (lhs.line == rhs.line)
        lhs.lastCol = rhs.lastCol;
    else //extend to end of line
        lhs.setLength(lhs.lineStr.length() - lhs.firstCol);
    return lhs;
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
