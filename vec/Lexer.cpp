#include "Lexer.h"
#include "Error.h"

#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cerrno>

using namespace lex;


Lexer::Lexer(std::string fname)
    : fileName(fname)
{
    std::ifstream t(fileName.c_str());
    t.seekg(0, std::ios::end);
    std::streamoff size = t.tellg();
    buffer.assign((size_t)size, ' ');
    t.seekg(0);
    t.read(&buffer[0], size);

    fileName = fileName.substr(fileName.find_last_of('\\') + 1);
    fileName = fileName.substr(fileName.find_last_of('/') + 1); //portability!

    curChr = &buffer[0] - 1; //will be incremented
    nextTok.loc.firstCol = nextTok.loc.lastCol = -1; //will be incremented
    nextTok.loc.line = 1; //lines are 1 based

    nextTok.loc.lineStr.assign(buffer.c_str());
    nextTok.loc.fileName = fileName.c_str();

    Advance();
}

tok::Token Lexer::Next()
{
    tok::Token thisTok = nextTok;
    Advance();
    return thisTok;
}

inline bool isWhitespace(char c)
{
    return c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == ' ';
}

inline void Lexer::lexChar(tok::TokenType type)
{
    nextTok.type = type;
    nextTok.loc.setLength(1);
}

inline void Lexer::lexBinOp(tok::TokenType type)
{
    if (curChr[1] == '=')
    {
        curChr++;
        nextTok.type = tok::opequals;
        nextTok.op = type;
        nextTok.loc.setLength(2);
    }
    else
    {
        nextTok.type = type;
        nextTok.loc.setLength(1);
    }
}

inline void Lexer::lexDouble(tok::TokenType type)
{
    curChr++;
    nextTok.type = type;
    nextTok.loc.setLength(2);
}

inline void Lexer::lexBinDouble(tok::TokenType bintype, tok::TokenType doubletype)
{
    if (curChr[1] == curChr[0])
        lexDouble(doubletype);
    else
        lexBinOp(bintype);
}

inline void Lexer::lexBinAndDouble(tok::TokenType bintype, tok::TokenType doubletype)
{
    if (curChr[1] == curChr[0]) //xx
    {
        if (curChr[2] == '=') // xx=. don't need to check if it exists, curChr[1] != 0 already
        {
            curChr += 2;
            nextTok.type = tok::opequals;
            nextTok.op = doubletype;
            nextTok.loc.setLength(3);
        }
        else //xx
            lexDouble(doubletype);
    }
    else //x
        lexBinOp(bintype);
}

namespace
{
    //expects curChr is A-Za-z. returns 1 past end (end iterator)
    inline const char * getEndOfWord(const char * begin)
    {
        const char * end = begin;

        while ((*end >= 'A' && *end <= 'Z') ||
                (*end >= 'a' && *end <= 'z') ||
                (*end >= '0' && *end <= '9'))
            ++end;

        return end;
    }
}

inline bool Lexer::lexKw(const char * kw, tok::TokenType kwtype)
{
    const char * end = getEndOfWord(curChr);

    size_t len = strlen(kw) > size_t(end - curChr) ? strlen(kw) : end - curChr;

    if (strncmp(curChr, kw, len) != 0) //it's not the keyword
        return false;

    nextTok.type = kwtype;
    nextTok.loc.setLength(len);
    curChr = end - 1;
    return true;
}

inline void Lexer::lexIdent()
{
    const char * end = getEndOfWord(curChr);
    nextTok.type = tok::identifier;
    nextTok.text.assign(curChr, end);
    nextTok.loc.setLength(nextTok.text.length());
    curChr = end - 1;
}

inline void Lexer::lexKwOrIdent(const char * kw, tok::TokenType kwtype)
{
    if (!lexKw(kw, kwtype))
        lexIdent();
}

inline void Lexer::lexNumber()
{
    const char * end = curChr; //where should it end?

    while ((*end >= '0' && *end <= '9')
        || *end == 'e' || *end == 'E'
        || *end == '+' || *end == '-'
        || *end == '.'
        || *end == 'x' || *end == 'X')
    {
        ++end;
    }
    
    // need this here for error printing
    nextTok.loc.setLength(end - curChr);

    errno = 0;
    char * convEnd; //where did it end?
    long l = strtol(curChr, (char**)&convEnd, 0);


    if (end == convEnd) //parsed long
    {
        //this is in here in case of 999999999999999999. for example
        if (errno == ERANGE) 
        {
            err::Error(err::error, nextTok.loc) << "integer constant out of range"
                << err::underline << err::endl;
            l = 0; //recover. fall thru to call it a long
        }

        //success

        nextTok.type = tok::integer;
        nextTok.value.int_v = l;
        curChr = end - 1;
        return;
    }

    //nope, could be float
    errno = 0;
    double d = strtod(curChr, (char**)&convEnd);

    if (end == convEnd) //success, parsed double
    {
        //this is in here in case of 9999999e999999e99999 for example
        if (errno == ERANGE)
        {
            err::Error(err::error, nextTok.loc) << "floating point constant out of range"
                << err::underline << err::endl;
            d = 0.; //recover. fall thru to call it a double
        }

        nextTok.type = tok::floating;
        nextTok.value.dbl_v = d;
        curChr = end - 1;
        return;
    }

    //nope, just junk
    err::Error(err::error, nextTok.loc) << "numeric constant not parseable"
        << err::underline << err::endl;
    nextTok.type = tok::integer; //recover
    nextTok.value.int_v = 0;
    curChr = end - 1;
}

inline void Lexer::lexChar()
{
    nextTok.type = tok::integer;
    nextTok.value.int_v = 0;

    //if it's '\EOF this falls thru to the else if which calls it a \
    //then misses the ending ' and finds it's unterminated
    if (curChr[1] == '\\' && curChr[2] != '\0') // '\x'
    {
        switch (curChr[2])
        {
        case 'n':
            nextTok.value.int_v = '\n';
            break;
        case 't':
            nextTok.value.int_v = '\t';
            break;
        case 'v':
            nextTok.value.int_v = '\v';
            break;
        case 'b':
            nextTok.value.int_v = '\b';
            break;
        case 'r':
            nextTok.value.int_v = '\r';
            break;
        case 'f':
            nextTok.value.int_v = '\f';
            break;
        case 'a':
            nextTok.value.int_v = '\a';
            break;
        case '0':
            nextTok.value.int_v = 0;
        default:
            err::Error(err::error, nextTok.loc) << "invalid escape sequence '\\"
                << curChr[2] << "' in character constant" << err::caret 
                << err::note << "special characters need not be escaped" << err::endl;
            break;
        }
        curChr += 3;
        nextTok.loc.setLength(4);
        if (*curChr == '\'') //otherwise, unterminated
            return;
    }
    else if (curChr[1] != '\0') // 'EOF  (to avoid out of bounds access to curChr[2])
    {
        nextTok.value.int_v = curChr[1];
        curChr += 2;
        nextTok.loc.setLength(3);
        if (*curChr == '\'') //otherwise, unterminated
            return;
    }
    
    const char * end = curChr;
    while (*end != '\'' && *end != '\n' && *end != '\0') //look for ' on rest of line
        ++end;

    if (*end == '\'') //found it
    {
        err::Error(err::error, nextTok.loc) << "character constant too long"
            << err::underline << err::endl;
    }
    else //didn't find it
    {
        err::Error(err::error, nextTok.loc) << "unterminated character constant"
            << err::caret << err::endl;
    }
    //recover
    nextTok.loc.setLength(nextTok.loc.getLength() + (end - curChr));
    curChr = (*end == '\0') ? end - 1 : end;
}

// the meat of the lexer
// at the start, curChr points to last char of last token
void Lexer::Advance()
{
    if (nextTok.type == tok::end) //can't advance
        return;

    nextTok.loc.firstCol = nextTok.loc.lastCol;

lexMore: //more elegant, in this case, than a while(true)

    ++nextTok.loc.firstCol;
    ++curChr;

    switch (*curChr)
    {
        //check for eof

    case '\0':
        nextTok.type = tok::end;
        return;

        //eat whitespace

    case '\n':
        ++nextTok.loc.line;
        nextTok.loc.firstCol = -TAB_SIZE; //to compensate for tab
        nextTok.loc.lineStr = utl::weak_string(curChr + 1);
    case '\t':
        nextTok.loc.firstCol += TAB_SIZE - 1; //add 3 extra spaces for tab
    case '\r':
    case '\v':
    case ' ':
        goto lexMore;

        //handle single characters

    case '`':
        lexBinOp(tok::tick);
        return;
    case '#':
        lexChar(tok::pound);
        return;
    case '(':
        lexChar(tok::lparen);
        return;
    case ')':
        lexChar(tok::rparen);
        return;
    case '{':
        lexChar(tok::lbrace);
        return;
    case '}':
        lexChar(tok::rbrace);
        return;
    case '[':
        lexChar(tok::lsquare);
        return;
    case ']':
        lexChar(tok::rsquare);
        return;
    case '\\':
        lexChar(tok::backslash);
        return;
    case ':':
        lexChar(tok::colon);
        return;
    case ';':
        lexChar(tok::semicolon);
        return;
    case ',':
        lexChar(tok::comma);
        return;
    case '.':
        if (curChr[1] >= '0' && curChr[1] <= '9') //number
            lexNumber();
        else //just a dot
            lexChar(tok::dot);
        return;
    case '?':
        lexChar(tok::question);
        return;

        //handle binary ops only ever followed by '='
    case '~':
        lexBinOp(tok::tilde);
        return;
    case '@':
        lexBinOp(tok::at);
        return;
    case '$':
        lexBinOp(tok::dollar);
        return;
    case '^':
        lexBinOp(tok::caret);
        return;
    case '*':
        lexBinOp(tok::star);
        return;

        //handle potential double characters, or x=

    case '+':
        lexBinDouble(tok::plus, tok::plusplus);
        return;
    case '-':
        lexBinDouble(tok::minus, tok::minusminus);
        return;

        //handle potential double characters, or x=, or xx=

    case '&':
        lexBinAndDouble(tok::amp, tok::ampamp);
        return;
    case '|':
        lexBinAndDouble(tok::bar, tok::barbar);
        return;

        //handle special cases

    case '<':
        if (curChr[1] == '=') //take care of this case here
            lexDouble(tok::notgreater);
        else
            lexBinAndDouble(tok::less, tok::lshift);
        return;
    case '>':
        if (curChr[1] == '=') //take care of this case here
            lexDouble(tok::notless);
        else
            lexBinAndDouble(tok::greater, tok::rshift);
        return;
    case '!':
        if (curChr[1] == '=')
            lexDouble(tok::notequals);
        else
            lexChar(tok::bang);
        return;
    case '=':
        if (curChr[1] == '=')
            lexDouble(tok::equalsequals);
        else
            lexChar(tok::equals);
        return;

    case '/':
        if (curChr[1] == '/') //c++ style comment
        {
            while (*curChr != '\n' && *curChr != '\0')
                curChr++;
            goto lexMore;
        }
        if (curChr[1] == '*') //c style comment
        {
            while (*curChr != '\0' && (*curChr != '*' || curChr[1] != '/'))
                curChr++;
            if (*curChr == '*')
                curChr++;
            goto lexMore;
        }
        lexChar(tok::slash);
        return;

    case '"':
        {
            curChr++;
            const char * end = curChr;
            while (*end != '"' && *end != '\0')
                end++;

            nextTok.loc.setLength(end - curChr + 2);

            if (*end == '\0')
            {
                err::Error(err::error, nextTok.loc) << "EOF in string constant"
                    << err::caret << err::endl;
                --end; //so as not to pass the end of the file
            }

            nextTok.text.assign(curChr, end);
            nextTok.type = tok::stringlit;
            curChr = end;
            return;
        }

    case '\'':
        lexChar();
        return;

            //handle keywords

    case 'b':
        lexKwOrIdent("break", tok::k_break);
        return;
    case 'c':
        if (lexKw("case", tok::k_case))
            return;
        if (lexKw("continue", tok::k_continue))
            return;
        lexKwOrIdent("c_call", tok::k_c_call);
        return;
    case 'd':
        if(lexKw("default", tok::k_default))
            return;
        lexKwOrIdent("do", tok::k_do);
        return;
    case 'e':
        lexKwOrIdent("else", tok::k_else);
        return;
    case 'f':
        if (lexKw("false", tok::k_false))
            return;
        lexKwOrIdent("for", tok::k_for);
        return;
    case 'i':
        if (lexKw("int", tok::k_int))
            return;
        if (lexKw("inline", tok::k_inline))
            return;
        lexKwOrIdent("if", tok::k_if);
        return;
    case 'r':
        lexKwOrIdent("return", tok::k_return);
        return;
    case 's':
        lexKwOrIdent("switch", tok::k_switch);
        return;
    case 't':
        if (lexKw("type", tok::k_type))
            return;
        lexKwOrIdent("true", tok::k_true);
        return;
    case 'w':
        lexKwOrIdent("while", tok::k_while);
        return;

    default:
        if (*curChr >= '0' && *curChr <= '9') //number
            lexNumber();
        else //identifier
            lexIdent();
        return;
    }
}
