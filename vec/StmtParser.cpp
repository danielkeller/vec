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
    | '(' ')'
    ;
*/
Expr* Parser::parseBlock()
{
    tok::Location begin = lexer->Next().loc;

    //check this before making a scope
    if (lexer->Peek() == tok::rparen) //empty parens
        return new NullExpr(begin + lexer->Next().loc);

    Scope blockScope(curScope);
    curScope = &blockScope;
    
    Expr* conts;

    while (true)
    {
        conts = parseExpression();

        //if parseExpression can't parse any more the user probably forgot a ;
        //so will you kindly insert it for them?
        if(!lexer->Expect(tok::rparen))
            err::ExpectedAfter(lexer, "';'", "expression");
        else
            break;
    }

    curScope = blockScope.getParent();
    return new Block(conts, std::move(blockScope), begin + lexer->Last().loc);
}

/*
expression
    : stmt-expr
    | stmt-expr ';' expression
    | stmt-expr ';' [followed by end of block]
    | stmt-expr ':' expression
    | 'case' stmt-expr ':' expression
    | 'default' ':' expression
    ;
*/
Expr* Parser::parseExpression()
{
    Expr* lhs = parseStmtExpr();

    if (lexer->Expect(tok::semicolon))
    {
        if (lexer->Peek() == tok::rparen || lexer->Peek() == tok::end)
            return lhs;

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
    tok::Location begin = lexer->Peek().loc;

    switch (lexer->Peek().type)
    {
    case tok::k_type:
        parseTypeDecl();
        return new NullExpr(begin + lexer->Last().loc);
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
        return new NullExpr(begin + lexer->Last().loc);
    default:
        return parseBinaryExpr();
    }
}
