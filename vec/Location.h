#ifndef LOCATION_H
#define LOCATION_H

#include "Util.h"
#include <ostream>

#define TAB_SIZE 8

namespace tok
{
    struct Location
    {
        int line;
        int firstCol, lastCol;

        utl::weak_string lineStr;
        
        const char * fileName; //char * instead of weak_string because we have nul terminator

        int printCaret(int start = 0);
        int printPostCaret(int start = 0);
        int printUnderline(int start = 0);

        void setLength(int nCols);
        int getLength() {return lastCol - firstCol + 1;};
    };

    Location operator+ (Location & lhs, Location & rhs);
    Location& operator+= (Location & lhs, Location & rhs);

    template <class CharT, class Traits>
    inline std::basic_ostream<CharT, Traits>& 
        operator<< (std::basic_ostream<CharT, Traits>& os, const Location& loc)
    {
        os << loc.fileName << ':' << loc.line << ':' << loc.firstCol;
        return os;
    }
}

#endif
