#include "Error.h"
#include "Location.h"
#include <iostream>

using namespace err;

#define COLOFF "\033[0m"
#define MGNTA "\033[1;35m"
#define RED "\033[1;31m"
#define YEL "\033[1;33m"

Error::Error(Level lvl, tok::Location &l)
    : posn(0), loc(&l)
{
    std::cerr << l << ": ";

    switch (lvl) //do error level filtering here
    {
    case fatal:
        std::cerr << MGNTA "fatal error: " COLOFF;
        break;

    case error:
        std::cerr << RED "error: " COLOFF;
        break;
        
    case warning:
    case nitpick:
        std::cerr << YEL "warning: " COLOFF;
        break;
    }
}

Error & Error::operator<< (Special toPrint)
{
    if (toPrint == endl)
    {
        std::cerr << std::endl << std::endl;
        return *this;
    }

    if (toPrint == note)
    {
        std::cerr << std::endl << *loc << ": note: ";
        posn = 0;
        return *this;
    }

    if (posn == 0) //if we're just starting, print the line
    {
        std::cerr << std::endl << loc->lineStr << std::endl;
    }

    switch (toPrint)
    {
    case caret:
        posn = loc->printCaret(std::cerr, posn);
        break;

    case underline:
        posn = loc->printUnderline(std::cerr, posn);
        break;

    default:
        break;
    }

    return *this;
}
