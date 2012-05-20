#include "Parser.h"
#include "Lexer.h"
#include "Error.h"
#include "CompUnit.h"
#include "Stmt.h"

using namespace par;
using namespace ast;

FuncBody* Parser::parseFuncBody()
{
    FuncBody * fb = new FuncBody(parseBlock());
    return fb;
}

Block* Parser::parseBlock()
{
    lexer->Advance();
    
    Expr* conts = parseExpression();
    if(!lexer->Expect(tok::rparen))
        err::ExpectedAfter(lexer, "')'", "block body");
    return new Block(conts);
}

Expr* Parser::parseExpression()
{
    Expr* lhs = parseStmtExpr();

    VarDef dummy;
    curScope->addVarDef(0, dummy);

    if (lexer->Expect(tok::semicolon))
    {
        Expr* rhs = parseExpression();
        return new SemiExpr(lhs, rhs);
    }
    else
        return lhs;
}

Expr* Parser::parseStmtExpr()
{
    return new NullExpr();
}
