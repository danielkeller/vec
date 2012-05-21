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

        //ExprParser.cpp
        ast::Expr* parseBinaryExpr();
        ast::Expr* parseBinaryExprRHS(ast::Expr* lhs, tok::prec::precidence minPrec);
        ast::Expr* parseUnaryExpr();

        //DeclParser.cpp
        void parseTypeDecl();
        ast::Expr* parseDecl();

        //StmtParser.cpp
        ast::Expr* parseExpression();
        ast::Block* parseBlock();
        ast::Expr* parseStmtExpr();

        friend class Type;
    };
}

#endif
