#include "Type.h"
#include "Lexer.h"
#include "Util.h"
#include "Error.h"

#include <stack>

using namespace typ;

namespace cod
{
    static const char list = 'L';
    static const char tuple = 'T';
    static const char tupend = 't';
    static const char integer = 'I';
    static const char floating = 'F';
    static const char ref = 'R';
    static const char named = 'N';
    static const char any = 'A';
    static const char param = 'P';
    static const char function = 'U';
}

bool typ::couldBeType(tok::Token &t)
{
    switch (t.Type())
    {
    case listBegin:
    case tupleBegin:
    case tok::k_int:
    case tok::k_float:
    case tok::question:
    case tok::at:
    case tok::identifier:
        return true;
    default:
        return false;
    }
}

Type::Type(lex::Lexer *l)
{
    parseSingle(l);
    if (l->Expect(tok::colon)) //function?
    {
        code += cod::function;
        parseSingle(l);
    }
}

void Type::parseSingle(lex::Lexer *l)
{
    tok::Token t = l->Peek();
    switch (t.Type())
    {
    case listBegin:
        parseList(l);
        break;

    case tupleBegin:
        parseTuple(l);
        break;

    case tok::k_int:
    case tok::k_float:
        parsePrim(l);
        break;

    case tok::question:
        parseParam(l);
        break;

    case tok::at:
        parseRef(l);
        break;

    case tok::identifier:
        parseNamed(l);
        break;

    default:
        err::Error(t.loc) << "unexpected " << t.Name() << ", expected type"
            << err::caret << err::endl;
        l->Advance();
    }
}

void Type::parseTypeList(lex::Lexer *l)
{
    while (true)
    {
        parseSingle(l);
        if (!l->Expect(tok::comma))
        {
            if (couldBeType(l->Peek()))
                err::Error(l->Peek().loc) << "missing comma in type list" << err::caret << err::endl;
            else
                break; //assume end of list
        }
    }
}

void Type::parseList(lex::Lexer *l)
{
    code += cod::list;
    tok::Location errLoc = l->Next().loc;
    parseSingle(l);
    if (!l->Expect(listEnd))
        err::Error(errLoc) << "unterminated list type" << err::caret << err::endl;
}

void Type::parseTuple(lex::Lexer *l)
{
    code += cod::tuple;
    tok::Location errLoc = l->Next().loc;
    parseTypeList(l);
    if (!l->Expect(tupleEnd))
        err::Error(errLoc) << "unterminated tuple type" << err::caret << err::endl;
    code += cod::tupend;
}

void Type::parsePrim(lex::Lexer *l)
{
    if (l->Next() == tok::k_int)
        code += cod::integer;
    else 
        code += cod::floating;
}

void Type::parseRef(lex::Lexer *l)
{
    code += cod::ref;
    l->Advance();
    parseSingle(l);
}

void Type::parseNamed(lex::Lexer *l)
{
    tok::Token t = l->Next();
    std::string str = t.text;
    code += cod::named + utl::to_str(str.length());
    code += str;
}

void Type::parseParam(lex::Lexer *l)
{
    l->Advance();
    if (l->Peek() == tok::identifier)
    {
        std::string str = l->Next().text;
        code += cod::param + utl::to_str(str.length());
        code += str;
    }
    else
        code += cod::any;
}

bool Type::isFunc()
{
    return code.find(cod::function) != std::string::npos;
}

bool Type::isTempl()
{
    return code.find(cod::any) != std::string::npos || code.find(cod::param) != std::string::npos;
}

utl::weak_string Type::w_str()
{
    return utl::weak_string(&*code.begin(), &*code.end());
}
