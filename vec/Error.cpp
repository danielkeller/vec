#include "Error.h"
#include "Location.h"
#include "Lexer.h"
#include "Global.h"
#include <iostream>

using namespace err;

void err::ExpectedAfter(lex::Lexer *l, const char *expected, const char *after)
{
    Error(l->Last().loc) << "expected " << expected << " after " << after << err::postcaret;
}

#ifdef _WIN32

#define COLOFF ""
#define MGNTA ""
#define RED ""
#define YEL ""
#define GRN ""
#define BOLD ""

#else

#define COLOFF "\033[0m"
#define MGNTA "\033[1;35m"
#define RED "\033[1;31m"
#define YEL "\033[1;33m"
#define GRN "\033[1;32m"
#define BOLD "\033[1m"

#endif

void Error::init(Level lvl)
{
    posn = 0;

    std::cerr << loc << ": ";

    switch (lvl) //do error level filtering here
    {
    case fatal:
        std::cerr << MGNTA "fatal error: " COLOFF BOLD;
        break;

    case error:
        ++Global().numErrors;
        std::cerr << RED "error: " COLOFF BOLD;
        break;

    case warning:
    case nitpick:
        std::cerr << YEL "warning: " COLOFF;
        break;
    }
}

Error::~Error()
{
    std::cerr << COLOFF "\n\n";
}

Error & Error::operator<< (const tok::Location &l)
{
    if (l.line != loc.line) //if its not on the same line, we need to print the new line
        posn = 0;

    loc = l;

    return *this;
}

Error & Error::operator<< (Special toPrint)
{
    std::cerr << COLOFF;

    if (toPrint == note)
    {
        std::cerr << '\n' << loc << ": note: ";
        posn = 0;
        return *this;
    }

    if (posn == 0) //if we're just starting, print the line
    {
        std::cerr << '\n' << loc.lineStr << '\n';
    }

    std::cerr << GRN;

    switch (toPrint)
    {
    case caret:
        posn = loc.printCaret(posn);
        break;

    case underline:
        posn = loc.printUnderline(posn);
        break;

    case postcaret:
        posn = loc.printPostCaret(posn);

    default:
        break;
    }

    std::cerr << COLOFF;

    return *this;
}
