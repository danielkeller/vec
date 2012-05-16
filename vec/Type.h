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
namespace ast
{
    class Scope;
}

namespace par
{
    class TypeParser;
    class TypeListParser;
}

namespace typ
{
    class Type
    {
    public:
        Type();
        utl::weak_string w_str();
        utl::weak_string ex_w_str();
        bool isFunc();
        bool isTempl();

    private:
        std::string code; //string representation of type - "nominative" type
        std::string expanded; //with all named types inserted - "duck" type

        friend class par::TypeListParser;
        friend class par::TypeParser;
    };
}

namespace par
{
    //so we can easily change the syntax if needed
    static const tok::TokenType listBegin = tok::lbrace, listEnd = tok::rbrace;
    static const tok::TokenType tupleBegin = tok::lsquare, tupleEnd = tok::rsquare;

    class TypeParser
    {
        typ::Type t;
        lex::Lexer *l;
        ast::Scope *s;
        
    public:
        typ::Type operator()(lex::Lexer *lex, ast::Scope *sc);

        void parseSingle();
        void parseTypeList();
        void parseList();
        void parseTuple();
        void parsePrim();
        void parseParam();
        void parseRef();
        void parseNamed();
        void parseIdent();
        size_t checkArgs();

        friend class TypeListParser;
    };

    bool couldBeType(tok::Token &t);
}

#endif
