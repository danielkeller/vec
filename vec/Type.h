#ifndef TYPE_H

#include <string>
#include "Token.h"

namespace lex
{
    class Lexer;
}
namespace utl
{
    class weak_string;
}

/*
Type codes are
    L - list
    T - tuple
    t - tuple end
    I - int
    F - float
    R - reference
    N - named N[length]name ie N3foo
    A - any
    P - param, "named any"
    U - function
*/

namespace typ
{
    //so we can easily change the syntax if needed
    static const tok::TokenType listBegin = tok::lbrace, listEnd = tok::rbrace;
    static const tok::TokenType tupleBegin = tok::lsquare, tupleEnd = tok::rsquare;

    class Type
    {
    public:
        Type(lex::Lexer *l); //extract next type from lexer
        utl::weak_string w_str();
        bool isFunc();
        bool isTempl();

    private:
        std::string code; //string representation of type
        Type(std::string &s) : code(s) {} //copy type from string

        void parseSingle(lex::Lexer *l);
        void parseTypeList(lex::Lexer *l);
        void parseList(lex::Lexer *l);
        void parseTuple(lex::Lexer *l);
        void parsePrim(lex::Lexer *l);
        void parseParam(lex::Lexer *l);
        void parseRef(lex::Lexer *l);
        void parseNamed(lex::Lexer *l);
    };

    bool couldBeType(tok::Token &t);
}

#define TYPE_H
#endif
