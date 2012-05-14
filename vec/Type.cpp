#include "Type.h"
#include "Lexer.h"
#include "Util.h"
#include "Error.h"

#include <stack>

using namespace typ;

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
    code += 'L';
    tok::Location errLoc = l->Next().loc;
    parseSingle(l);
    if (!l->Expect(listEnd))
        err::Error(errLoc) << "unterminated list type" << err::caret << err::endl;
}

void Type::parseTuple(lex::Lexer *l)
{
    code += 'T';
    tok::Location errLoc = l->Next().loc;
    parseTypeList(l);
    if (!l->Expect(tupleEnd))
        err::Error(errLoc) << "unterminated tuple type" << err::caret << err::endl;
    code += 't';
}

void Type::parsePrim(lex::Lexer *l)
{
    if (l->Next() == tok::k_int)
        code += 'I';
    else 
        code += 'F';
}

void Type::parseRef(lex::Lexer *l)
{
    code += 'R';
    l->Advance();
    parseSingle(l);
}

void Type::parseNamed(lex::Lexer *l)
{
    tok::Token t = l->Next();
    std::string str = t.text;
    code += 'N' + utl::to_str(str.length());
    code += str;
}

void Type::parseParam(lex::Lexer *l)
{
    code += '?';
    if (l->Expect(tok::identifier))
        parseNamed(l);
}

utl::weak_string Type::w_str()
{
    return utl::weak_string(&*code.begin(), &*code.end());
}
