#ifndef LOCATION_H

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

        template <class CharT, class Traits>
        int printCaret(std::basic_ostream<CharT, Traits>& os, int start = 0);

        template <class CharT, class Traits>
        int printUnderline(std::basic_ostream<CharT, Traits>& os, int start = 0);

        void setLength(int nCols);
        int getLength() {return lastCol - firstCol + 1;};
    };

    inline Location operator+ (Location & lhs, Location & rhs);

    template <class CharT, class Traits>
    int Location::printCaret(std::basic_ostream<CharT, Traits>& os, int start)
    {
        int i;
        for (i = start; i < firstCol; ++i)
            os.put(' ');
        os.put('^');

        return i + 1;
    }

    template <class CharT, class Traits>
    int Location::printUnderline(std::basic_ostream<CharT, Traits>& os, int start)
    {
        int i;
        for (i = start; i < firstCol; ++i)
            os.put(' ');
        for (; i <= lastCol; ++i)
            os.put('~');

        return i;
    }

    template <class CharT, class Traits>
    inline std::basic_ostream<CharT, Traits>& 
        operator<< (std::basic_ostream<CharT, Traits>& os, const Location& loc)
    {
        os << loc.fileName << ':' << loc.line << ':' << loc.firstCol;
        return os;
    }
}

#define LOCATION_H
#endif