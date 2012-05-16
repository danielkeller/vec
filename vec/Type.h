#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <vector>
#include "Token.h"

namespace lex
{
    class Lexer;
}
namespace utl
{
    class weak_string;
}

namespace typ
{
    //so we can easily change the syntax if needed
    static const tok::TokenType listBegin = tok::lbrace, listEnd = tok::rbrace;
    static const tok::TokenType tupleBegin = tok::lsquare, tupleEnd = tok::rsquare;

    class Type
    {
    public:
        Type(lex::Lexer *l); //extract next type from lexer
        Type();
        utl::weak_string w_str();
        utl::weak_string ex_w_str();
        bool isFunc();
        bool isTempl();

    private:
        std::string code; //string representation of type - "nominative" type
        std::string expanded; //with all named types inserted - "duck" type
        Type(std::string &s) : code(s) {} //copy type from string

        void parseSingle(lex::Lexer *l);
        void parseTypeList(lex::Lexer *l);
        void parseList(lex::Lexer *l);
        void parseTuple(lex::Lexer *l);
        void parsePrim(lex::Lexer *l);
        void parseParam(lex::Lexer *l);
        void parseRef(lex::Lexer *l);
        void parseNamed(lex::Lexer *l);
        void parseIdent(lex::Lexer *l);
        void checkArgs(lex::Lexer *l);

        friend class TypeListParser;
    };

    bool couldBeType(tok::Token &t);
}

#endif
