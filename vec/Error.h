#include <iostream>

namespace tok
{
    struct Location;
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
        underline,
        note,
        endl
    };

    class Error
    {
        int posn;
        tok::Location * loc;

    public:
        Error(Level lvl, tok::Location &loc);

        template<class T>
        Error & operator<< (T toPrint);
        
        Error & operator<< (Special toPrint);
        
        Error & operator<< (tok::Location &l) //set new location
            {loc = &l; return *this;}
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
