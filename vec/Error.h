#ifndef ERROR_H
#define ERROR_H

#include <iostream>

#include "Location.h"

namespace tok
{
    struct Location;
}

namespace lex
{
    struct Lexer;
}

namespace err
{
    enum Level
    {
        fatal,
        error,
        warning,
        nitpick,
    };

    enum Special
    {
        caret,
        postcaret,
        underline,
        note,
        endl
    };

    void ExpectedAfter(lex::Lexer *l, const char *expected, const char *after);

    class Error
    {
        int posn;
        tok::Location loc;

        void init(Level lvl);

    public:
        Error(Level lvl, tok::Location loc);
        Error(tok::Location loc);

        template<class T>
        Error & operator<< (T toPrint);
        
        Error & operator<< (Special toPrint);
        
        Error & operator<< (tok::Location l); //set new location
    };

    template<class T>
    Error & Error::operator<< (T toPrint)
    {
        if (posn != 0) //just printed location info, print newline
        {
            std::cerr << std::endl;
            posn = 0;
        }

        std::cerr << toPrint;
        return *this;
    }
}

#endif
