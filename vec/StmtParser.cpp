#include "Parser.h"
#include "Lexer.h"
#include "Error.h"
#include "CompUnit.h"
#include "Stmt.h"

using namespace par;
using namespace ast;

/*
primary-expr
    : '(' stmt-list ')'
    | '(' ')'
    ;
*/
Block* Parser::parseBlock()
{
    tok::Location begin = lexer->Next().loc;

    cu->scopes.emplace_back(curScope);
    Scope* blockScope = curScope = &cu->scopes.back();

    if (lexer->Expect(tok::rparen)) //empty parens
        return new Block(new NullStmt(begin + lexer->Last().loc),
                         blockScope,
                         begin + lexer->Last().loc);
    
    Stmt* conts;

    conts = parseStmtList();

    if(!lexer->Expect(tok::rparen))
        err::ExpectedAfter(lexer, "')'", "block");

    curScope = curScope->getParent();
    return new Block(conts, blockScope, begin + lexer->Last().loc);
}

/*
stmt-list
    : stmt
    | stmt stmt-list
    | expression ':' stmt-list
    | 'case' expression ':' stmt-list
    | 'default' ':' stmt-list
    ;
*/
Stmt* Parser::parseStmtList()
{
    Stmt* lhs = parseStmt();

    //if (!lexer->Expect(tok::semicolon) && lexer->Peek() != tok::rparen)
    //    err::ExpectedAfter(lexer, "';'", "expression");

    if (!(lexer->Peek() == tok::rparen || lexer->Peek() == tok::end))
    {
        Stmt* rhs = parseStmtList();
        return new StmtPair(lhs, rhs);
    }
    else //if (lexer->Peek() == tok::rparen || lexer->Peek() == tok::end)
        return lhs;

}

/*
stmt
    : expression ';'
    | expression [followed by end of block]
    | 'while' '(' expression ')' stmt-expr
    | 'if' '(' expression ')' stmt-expr
    | 'if' '(' expression ')' stmt-expr 'else' stmt-expr
    | 'switch' '(' expression ')' block
    | 'return' expression ';'
    | type-decl
    ;
*/
Stmt* Parser::parseStmt()
{
    tok::Token to = lexer->Peek();

    switch (lexer->Peek().type)
    {
    case tok::k_type:
        parseTypeDecl();
        return new NullStmt(to.loc + lexer->Last().loc);

    case tok::k_for:
    case tok::k_while:
    {
        lexer->Advance();
        if (!lexer->Expect(tok::lparen))
            err::ExpectedAfter(lexer, "'('", "'while'");
        Expr* pred = parseExpression();
        if (!lexer->Expect(tok::rparen))
            err::ExpectedAfter(lexer, "')'", "expression");
        Stmt* act = parseStmt();
        return new WhileStmt(pred, act, to);
    }

    case tok::k_if:
    {
        lexer->Advance();
        if (!lexer->Expect(tok::lparen))
            err::ExpectedAfter(lexer, "'('", "'if'");
        Expr* pred = parseExpression();
        if (!lexer->Expect(tok::rparen))
            err::ExpectedAfter(lexer, "')'", "expression");
        Stmt* act1 = parseStmt();
        if (!lexer->Expect(tok::k_else))
            return new IfStmt(pred, act1, to);
        else
            return new IfElseStmt(pred, act1, parseStmt(), to);
    }

    case tok::k_switch:
    {
        lexer->Advance();
        if (!lexer->Expect(tok::lparen))
            err::ExpectedAfter(lexer, "'('", "'switch'");
        Expr* pred = parseExpression();
        if (!lexer->Expect(tok::rparen))
            err::ExpectedAfter(lexer, "')'", "expression");
        Stmt* act = parseBlock();
        return new SwitchStmt(pred, act, to);
    }

    case tok::k_break:
    case tok::k_continue:
        err::Error(to.loc) << "I can't deal with " << to.Name() << err::underline;
        exit(-1);
    case tok::k_return:
    {
        lexer->Advance();
        Stmt* ret = new ReturnStmt(parseExpression(), to);
        if (!lexer->Expect(tok::semicolon) && lexer->Peek() != tok::rparen)
            err::ExpectedAfter(lexer, "';'", "expression");
        return ret;
    }
    case tok::k_tail:
    case tok::k_goto:
        //parse jump expr
        err::Error(to.loc) << "I can't deal with " << to.Name() << err::underline;
        exit(-1);
    default:
    {
        Stmt* ret = new ExprStmt(parseExpression());
        if (!lexer->Expect(tok::semicolon) && lexer->Peek() != tok::rparen && lexer->Last() != tok::rparen)
            err::ExpectedAfter(lexer, "';'", "expression");
        return ret;
    }
    }
}
