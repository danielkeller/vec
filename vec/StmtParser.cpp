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

    conts = parseExpression();

    if(!lexer->Expect(tok::rparen))
        err::ExpectedAfter(lexer, "')'", "block");

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

    //if (!lexer->Expect(tok::semicolon) && lexer->Peek() != tok::rparen)
    //    err::ExpectedAfter(lexer, "';'", "expression");

    if (lexer->Expect(tok::semicolon) && //eat the ; anyway even if we don't need it
        !(lexer->Peek() == tok::rparen || lexer->Peek() == tok::end))
    {
        Expr* rhs = parseExpression();
        return new SemiExpr(lhs, rhs);
    }
    else //if (lexer->Peek() == tok::rparen || lexer->Peek() == tok::end)
        return lhs;

}

/*
stmt-expr
    : comma-expr
    | 'while' '(' expression ')' stmt-expr
    | 'if' '(' expression ')' stmt-expr
    | 'if' '(' expression ')' stmt-expr 'else' stmt-expr
    | 'switch' '(' expression ')' block
    | 'return' stmt-expr
    | type-decl
    ;
*/
Expr* Parser::parseStmtExpr()
{
    tok::Token to = lexer->Peek();

    switch (lexer->Peek().type)
    {
    case tok::k_type:
        parseTypeDecl();
        return new NullExpr(to.loc + lexer->Last().loc);

    case tok::k_for:
    case tok::k_while:
    {
        lexer->Advance();
        if (!lexer->Expect(tok::lparen))
            err::ExpectedAfter(lexer, "'('", "'while'");
        Expr* pred = parseExpression();
        if (!lexer->Expect(tok::rparen))
            err::ExpectedAfter(lexer, "')'", "expression");
        Expr* act = parseStmtExpr();
        return new WhileExpr(pred, act, to);
    }
        //parse loop expr

    case tok::k_if:
    {
        lexer->Advance();
        if (!lexer->Expect(tok::lparen))
            err::ExpectedAfter(lexer, "'('", "'if'");
        Expr* pred = parseExpression();
        if (!lexer->Expect(tok::rparen))
            err::ExpectedAfter(lexer, "')'", "expression");
        Expr* act1 = parseStmtExpr();
        if (!lexer->Expect(tok::k_else))
            return new IfExpr(pred, act1, to);
        else
            return new IfElseExpr(pred, act1, parseStmtExpr(), to);
    }

    case tok::k_switch:
    {
        lexer->Advance();
        if (!lexer->Expect(tok::lparen))
            err::ExpectedAfter(lexer, "'('", "'switch'");
        Expr* pred = parseExpression();
        if (!lexer->Expect(tok::rparen))
            err::ExpectedAfter(lexer, "')'", "expression");
        Expr* act = parseBlock();
        return new SwitchExpr(pred, act, to);
    }

    case tok::k_break:
    case tok::k_continue:
        err::Error(to.loc) << "I can't deal with " << to.Name() << err::underline << err::endl;
        exit(-1);
    case tok::k_return:
        lexer->Advance();
        return new ReturnExpr(parseStmtExpr(), to);
    case tok::k_tail:
    case tok::k_goto:
        //parse jump expr
        err::Error(to.loc) << "I can't deal with " << to.Name() << err::underline << err::endl;
        exit(-1);
    default:
        return parseBinaryExpr();
    }
}
