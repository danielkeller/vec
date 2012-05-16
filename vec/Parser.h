#ifndef PARSER_H
#define PARSER_H

#include "Type.h"

namespace ast
{
    class CompUnit;
    class Scope;
}

namespace lex
{
    class Lexer;
}

namespace par
{
    class Parser
    {
    public:
        Parser(lex::Lexer *l);

    private:
        lex::Lexer *lexer;

        ast::CompUnit *cu;
        ast::Scope *curScope;

        TypeParser tp;

        void parseGlobalDecl();
        void parseTypeDecl();
        void parseDecl();

        friend class Type;
    };
}

#endif
