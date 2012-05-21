#include "Parser.h"
#include "Lexer.h"
#include "Error.h"
#include "CompUnit.h"
#include "Stmt.h"

using namespace par;
using namespace ast;

/*
primary-expr
    : '(' expression ')'
    ;
*/
Block* Parser::parseBlock()
{
    lexer->Advance();
    
    Expr* conts = parseExpression();
    if(!lexer->Expect(tok::rparen))
        err::ExpectedAfter(lexer, "')'", "block body");
    return new Block(conts);
}

/*
expression
    : stmt-expr
    | stmt-expr ';' expression
    | stmt-expr ':' expression
    | 'case' stmt-expr ':' expression
    ;
*/
Expr* Parser::parseExpression()
{
    Expr* lhs = parseStmtExpr();

    if (lexer->Expect(tok::semicolon))
    {
        Expr* rhs = parseExpression();
        return new SemiExpr(lhs, rhs);
    }
    else
        return lhs;
}

/*
stmt-expr
    : comma-expr
    | loop-expr
    | selection-expr
    | jump-expr
    | type-decl
    ;
*/
Expr* Parser::parseStmtExpr()
{
    switch (lexer->Peek().type)
    {
    case tok::k_type:
        parseTypeDecl();
        return new NullExpr();
    case tok::k_for:
    case tok::k_while:
        //parse loop expr
    case tok::k_if:
    case tok::k_else:
    case tok::k_switch:
        //parse selection expr
    case tok::k_break:
    case tok::k_continue:
    case tok::k_return:
    case tok::k_goto:
        //parse jump expr
        return new NullExpr();
    default:
        return parseBinaryExpr();
    }
}
