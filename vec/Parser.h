#ifndef PARSER_H
#define PARSER_H

#include "Type.h"
#include "CompUnit.h"
#include "Lexer.h"
#include "Stmt.h"

namespace par
{
    //so we can easily change the syntax if needed
    static const tok::TokenType listBegin = tok::lbrace, listEnd = tok::rbrace;
    static const tok::TokenType tupleBegin = tok::lsquare, tupleEnd = tok::rsquare;

    bool couldBeType(tok::Token &t);

    class Parser
    {
    public:
        Parser(lex::Lexer *l);

    private:
        lex::Lexer *lexer;

        ast::CompUnit *cu;
        ast::Scope *curScope;

        typ::Type type;

        //ExprParser.cpp
        ast::Expr* parseExpression();
        ast::Expr* parseBinaryExprInAgg();
        ast::Expr* parseBinaryExprRHS(ast::Expr* lhs, tok::prec::precidence minPrec);
        ast::Expr* parseUnaryExpr();
        ast::Expr* parsePostfixExpr();
        ast::Expr* parsePrimaryExpr();

        //DeclParser.cpp
        void parseTypeDecl();
        ast::Expr* parseDecl();

        //StmtParser.cpp
        ast::Stmt* parseStmtList();
        ast::Block* parseBlock();
        ast::Stmt* parseStmt();

        //TypeParser.cpp
        void parseType();
        void parseSingle();
        void parseTypeList();
        /*
        //parse list type or listify ie {int} or {1, 2, 3}
        ast::Expr* parseListOrIfy();
        ast::Expr* parseTupleOrIfy();
        */
        void parseList();
        void parseTuple();
        void parsePrim();
        void parseParam();
        void parseRef();
        void parseNamed();
        void parseIdent();
        size_t checkArgs();

        friend class TypeListParser;
        friend class Type;
    };
}

#endif
