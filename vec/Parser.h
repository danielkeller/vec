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
        ast::BlockScope bs;
        ast::FuncBody *curFunc;

        TypeParser tp;

        //ExprParser.cpp
        ast::Expr* parseExpression();

        //DeclParser.cpp
        void parseTypeDecl();
        ast::Expr* parseDecl();
        ast::FuncBody* parseFuncBody();

        //StmtParser.cpp
        ast::Block* parseBlock();
        ast::Expr* parseStmtExpr();

        friend class Type;
    };
}

#endif
