#ifndef PARSER_H
#define PARSER_H

#include "Type.h"
#include "Module.h"
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

        ast::Module *mod;
        ast::Scope *curScope;

        typ::Type type;

        //Node0Parser.cpp
        ast::Node0* parseExpression();
        ast::Node0* parseBinaryExprInAgg();
        ast::Node0* parseBinaryExprRHS(ast::Node0* lhs, tok::prec::precidence minPrec);
        ast::Node0* parseUnaryExpr();
        ast::Node0* parsePostfixExpr();
        ast::Node0* parsePrimaryExpr();
        //parse list type or listify ie {int} or {1, 2, 3}
        //returns NULL if type is encountered
        //this is needed because we don't know what { means w/o looking at its contents
        ast::Node0* parseListify();
        ast::Node0* parseTuplify();

        //DeclParser.cpp
        void parseTypeDecl();
        ast::Node0* parseDecl();
        ast::Node0* parseDeclRHS(); //parse the ident part of a decl, after the type has been parsed

        //StmtParser.cpp
        ast::Node0* parseStmtList();
        ast::Block* parseBlock();
        ast::Node0* parseStmt();

        //TypeParser.cpp
        void parseType();
        void parseTypeEnd(); //parse possible function type at end of top level type
        void parseSingle();
        void parseTypeList();
        //use these when we only want a type
        void parseList();
        void parseTuple();
        void parsePrim();
        void parseParam();
        void parseRef();
        void parseNamed();

        enum {
            CantBacktrack,
            CanBacktrack,
            IsBacktracking
        } backtrackStatus;

        friend class typ::Type;
    };
}

#endif
