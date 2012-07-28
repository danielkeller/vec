#include "Parser.h"
#include "Lexer.h"
#include "Error.h"
#include "Module.h"
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

    NormalScope* oldScope = curScope;
    mod->scopes.emplace_back(curScope);
    NormalScope* blockScope = curScope = &mod->scopes.back();

    if (lexer->Expect(tok::rparen)) //empty parens
        return new Block(Ptr(new NullStmt(begin + lexer->Last().loc)),
                         blockScope,
                         begin + lexer->Last().loc);
    
    Node0* conts;

    conts = parseStmtList();

    if(!lexer->Expect(tok::rparen))
        err::ExpectedAfter(lexer, "')'", "block");

    curScope = oldScope;
    return new Block(Ptr(conts), blockScope, begin + lexer->Last().loc);
}

/*
stmt-list
    : stmt
    | stmt stmt-list
    | expression ':' stmt-list
    | 'case' expression ':' stmt-list
    | 'default' ':' stmt-list
    | 'private' ':' stmt-list //only in global scope
    ;
*/
Node0* Parser::parseStmtList()
{
    tok::Token label = lexer->Peek();

    bool wantColon = true;

    switch (label.type)
    {
    case tok::k_private:
        if (curScope == &mod->pub)
            curScope = &mod->priv;
        else
            err::Error(label.loc) << "'private' not allowed in this scope" << err::underline;
        lexer->Advance();
        break;

    default:
        wantColon = false;
    }

    if (wantColon && !lexer->Expect(tok::colon))
        err::ExpectedAfter(lexer, ":", "label");

    Node0* lhs = parseStmt();

    if (!(lexer->Peek() == tok::rparen || lexer->Peek() == tok::end))
    {
        Node0* rhs = parseStmtList();
        return new StmtPair(Ptr(lhs), Ptr(rhs));
    }
    else
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
Node0* Parser::parseStmt()
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
        Node0* pred = parseExpression();
        if (!lexer->Expect(tok::rparen))
            err::ExpectedAfter(lexer, "')'", "expression");
        Node0* act = parseStmt();
        return new WhileStmt(Ptr(pred), Ptr(act), to);
    }

    case tok::k_if:
    {
        lexer->Advance();
        if (!lexer->Expect(tok::lparen))
            err::ExpectedAfter(lexer, "'('", "'if'");
        Node0* pred = parseExpression();
        if (!lexer->Expect(tok::rparen))
            err::ExpectedAfter(lexer, "')'", "expression");
        Node0* act1 = parseStmt();
        if (!lexer->Expect(tok::k_else))
            return new IfStmt(Ptr(pred), Ptr(act1), to);
        else
            return new IfElseStmt(Ptr(pred), Ptr(act1), Ptr(parseStmt()), to);
    }

    case tok::k_switch:
    {
        lexer->Advance();
        if (!lexer->Expect(tok::lparen))
            err::ExpectedAfter(lexer, "'('", "'switch'");
        Node0* pred = parseExpression();
        if (!lexer->Expect(tok::rparen))
            err::ExpectedAfter(lexer, "')'", "expression");
        Node0* act = parseBlock();
        return new SwitchStmt(Ptr(pred), Ptr(act), to);
    }

    case tok::k_break:
    case tok::k_continue:
        err::Error(to.loc) << "I can't deal with " << to.Name() << err::underline;
        exit(-1);
    case tok::k_return:
    {
        lexer->Advance();
        Node0* ret = new ReturnStmt(Ptr(parseExpression()), to);
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
        Node0* ret = new ExprStmt(Ptr(parseExpression()));
        if (!lexer->Expect(tok::semicolon) && lexer->Peek() != tok::rparen && lexer->Last() != tok::rparen)
            err::ExpectedAfter(lexer, "';'", "expression");
        return ret;
    }
    }
}
