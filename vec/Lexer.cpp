#include "Lexer.h"
#include "Error.h"
#include "CompUnit.h"

#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <algorithm>

//MSVC doesn't have strtoll / strtold
#ifdef _WIN32
#define strtoll _strtoi64
#define strtold strtod //no replacement?
#endif

using namespace lex;


Lexer::Lexer(std::string fname, ast::CompUnit *cu)
    : fileName(fname), compUnit(cu)
{
    std::ifstream t(fileName.c_str(), std::ios_base::in | std::ios_base::binary); //to keep CR / CRLF from messing us up

    if (!t.is_open())
    {
        //this doesnt work for some reason?
        err::Error(tok::Location()) << "Could not open file '" << fileName << '\'';
        exit(-1);
    }

    t.seekg(0, std::ios::end);
    std::streamoff size = t.tellg();
    buffer = new char[(size_t)size + 1];

    t.seekg(0);
    t.read(buffer, size);
    buffer[size] = 0;

    fileName = fileName.substr(fileName.find_last_of('\\') + 1);
    fileName = fileName.substr(fileName.find_last_of('/') + 1); //portability!

    curChr = buffer - 1; //will be incremented
    nextTok.loc.firstCol = nextTok.loc.lastCol = -1; //will be incremented
    nextTok.loc.line = 1; //lines are 1 based

    nextTok.loc.lineStr.assign(buffer);
    nextTok.loc.fileName = fileName.c_str();

    Advance();
}

tok::Token & Lexer::Next()
{
    Advance();
    return curTok;
}

bool Lexer::Expect(tok::TokenType t)
{
    if (Peek() != t)
        return false;

    Advance();
    return true;
}

bool Lexer::Expect(tok::TokenType t, tok::Token &to)
{
    if (Peek() != t)
        return false;

    to = Next();
    return true;
}

void Lexer::ErrUntil(tok::TokenType t)
{
    while (curTok != t && nextTok != tok::end)
        Advance();
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
        nextTok.value.op = type;
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
            nextTok.value.op = doubletype;
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
    std::string idname(curChr, end);
    nextTok.value.ident_v = compUnit->addIdent(idname);
    nextTok.loc.setLength(end - curChr);
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
    long long l = strtoll(curChr, (char**)&convEnd, 0);


    if (end == convEnd) //parsed long
    {
        //this is in here in case of 999999999999999999. for example
        if (errno == ERANGE)
        {
            err::Error(nextTok.loc) << "integer constant out of range" << err::underline;
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
    long double d = strtold(curChr, (char**)&convEnd);

    if (end == convEnd) //success, parsed double
    {
        //this is in here in case of 9999999e999999e99999 for example
        if (errno == ERANGE)
        {
            err::Error(nextTok.loc) << "floating point constant out of range" << err::underline;
            d = 0.; //recover. fall thru to call it a double
        }

        nextTok.type = tok::floating;
        nextTok.value.dbl_v = d;
        curChr = end - 1;
        return;
    }

    //nope, just junk
    err::Error(nextTok.loc) << "numeric constant not parseable" << err::underline;
    nextTok.type = tok::integer; //recover
    nextTok.value.int_v = 0;
    curChr = end - 1;
}

inline char Lexer::consumeChar()
{
    char ret = 0;
    if (curChr[0] == '\\' && curChr[1] != '\0') // '\x'
    {
        switch (curChr[1])
        {
        case 'n':
            ret = '\n';
            break;
        case 't':
            ret = '\t';
            break;
        case 'v':
            ret = '\v';
            break;
        case 'b':
            ret = '\b';
            break;
        case 'r':
            ret = '\r';
            break;
        case 'f':
            ret = '\r';
            break;
        case 'a':
            ret = '\a';
            break;
        case '0':
            ret = '\0';
            break;
        case '\'':
            ret = '\'';
            break;
        case '\"':
            ret = '\"';
            break;
        case '\\':
            ret = '\\';
            break;
        default:
            tok::Location escLoc = nextTok.loc;
            escLoc.firstCol = nextTok.loc.lastCol + 1;
            err::Error(err::error, escLoc) << "invalid escape sequence '\\"
                << curChr[1] << '\'' << err::caret;
            ret = 0;
            break;
        }
        curChr += 2;
        nextTok.loc.lastCol += 2;
        return ret;
    }
    else if (curChr[0] != '\0')
    {
        ret = curChr[0];
        curChr++;
        nextTok.loc.lastCol++;
        return ret;
    }

    return 0;
}

inline void Lexer::lexChar()
{
    nextTok.type = tok::integer;

    curChr++; //skip over '
    nextTok.loc.lastCol = nextTok.loc.firstCol + 1;
    nextTok.value.int_v = consumeChar();

    if (*curChr == '\'') //otherwise, unterminated
        return;

    const char * end = curChr;
    while (*end != '\'' && *end != '\n' && *end != '\0') //look for ' on rest of line
        ++end;

    nextTok.loc.setLength(nextTok.loc.getLength() + (end - curChr));
    if (*end == '\'') //found it
    {
        err::Error(nextTok.loc) << "character constant too long" << err::underline;
    }
    else //didn't find it
    {
        err::Error(nextTok.loc) << "unterminated character constant" << err::caret;
    }
    //recover
    curChr = (*end == '\0') ? end - 1 : end;
}

inline void Lexer::lexString()
{
    nextTok.type = tok::stringlit;

    curChr++; //skip over "
    nextTok.loc.lastCol = nextTok.loc.firstCol + 1;

    std::string strdata;
    while (*curChr != '\"' && *curChr != '\0')
        strdata += consumeChar();

    if (*curChr == '\0')
        err::Error(nextTok.loc) << "eof in string literal" << err::caret;

    //insert string as new entry only if it's not already there
    nextTok.value.int_v = compUnit->addString(strdata);
}

// the meat of the lexer
// at the start, curChr points to last char of last token
void Lexer::Advance()
{
    if (nextTok.type == tok::end) //can't advance
        return;

    curTok = nextTok;

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
            while (curChr[1] != '\n' && curChr[1] != '\0')
                curChr++;
            goto lexMore;
        }
        if (curChr[1] == '*') //c style comment
        {
            while (*curChr != '\0' && (*curChr != '*' || curChr[1] != '/'))
            {
                ++nextTok.loc.firstCol;
                if (*curChr == '\n')
                {
                    ++nextTok.loc.line;
                    nextTok.loc.firstCol = 0;
                    nextTok.loc.lineStr = utl::weak_string(curChr + 1);
                }
                else if (*curChr == '\t')
                    nextTok.loc.firstCol += TAB_SIZE;
                ++curChr;
            }
            if (*curChr == '*')
            {
                ++nextTok.loc.firstCol;
                ++curChr;
            }
            goto lexMore;
        }
        lexChar(tok::slash);
        return;

    case '"':
        lexString();
        return;

    case '\'':
        lexChar();
        return;

            //handle keywords

    case 'a':
        lexKwOrIdent("agg", tok::k_agg);
        return;
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
        if (lexKw("float", tok::k_float))
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
        if (lexKw("tail", tok::k_tail))
            return;
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
