#ifndef PARSER_H
#define PARSER_H

#include "Type.h"
#include "CompUnit.h"
#include "Lexer.h"
#include "Stmt.h"

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

        ast::FuncBody* parseFuncBody();

        ast::Block* parseBlock();

        friend class Type;
    };
}

#endif
