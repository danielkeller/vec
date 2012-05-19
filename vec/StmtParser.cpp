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
    if (lexer->Expect(tok::lparen))
    {
        //parse list of control exprs
        if(!lexer->Expect(tok::rparen))
            err::ExpectedAfter(lexer, "')'", "block body");
    }
    else
        ;
        //parseControlExpr()
    return new Block(cu->makeScope());
}
