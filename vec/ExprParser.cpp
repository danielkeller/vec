#include "Parser.h"
#include "Lexer.h"
#include "Error.h"
#include "CompUnit.h"
#include "Stmt.h"

using namespace par;
using namespace ast;

//big ole binary expression parser right here
//uses token precidence and associativity to parse it all in one function
Expr* Parser::parseExpression()
{
    Expr* lhs = parseUnaryExpr();
    return parseBinaryExprRHS(lhs, tok::prec::comma); //minumun prec is comma so ';' and stuff makes it return
}

/*
agg-expr
    : assignment-expr
    | AGG_OP agg-expr
    ;
*/
//needed because agg unary expr has lower precidence than assignment
Expr* Parser::parseBinaryExprInAgg()
{
    Expr* lhs = parseUnaryExpr();
    return parseBinaryExprRHS(lhs, tok::prec::assignment);
}

Expr* Parser::parseBinaryExprRHS(Expr* lhs, tok::prec::precidence minPrec)
{
    while(lexer->Peek().Precidence() >= minPrec) //keep shifting?
    {
        tok::Token op = lexer->Next(); //we know its a bin op because others are prec::none

        Expr* rhs = parseUnaryExpr();

        //shift
        rhs = parseBinaryExprRHS(rhs, tok::prec::precidence(op.Precidence()
            + (op.Asso_ty() == tok::LeftAssoc ? 1 : 0)));
        //precidence can be the same if it is right associative

        //reduce
        err::Error(err::warning, lhs->loc) << "wheee expression" << err::underline << op.loc << err::caret
            << rhs->loc << err::underline << err::endl;
        lhs = makeBinExpr(lhs, rhs, op);
        //fall through and "tail recurse"
    }
    return lhs; //cave johnson, we're done here
}

/*
agg-expr
    : assignment-expr
    | AGG_OP agg-expr
    ;

unary-expr
    : postfix-expr
    | UN_OP unary-expr
    ;
*/
Expr* Parser::parseUnaryExpr()
{
    tok::Token op = lexer->Peek();
    switch (op.type)
    {
    case tok::tilde:
    case tok::bang:
    case tok::minus:
    case tok::plus:
    case tok::plusplus:
    case tok::minusminus:
        lexer->Advance();
        return new UnExpr(parsePostfixExpr(), op);
    case tok::tick:
        lexer->Advance();
        return new IterExpr(parsePostfixExpr(), op);
    case tok::opequals:
        lexer->Advance();
        return new AggExpr(parseBinaryExprInAgg(), op);
    default:
        return parsePostfixExpr();
    }
}

/*
postfix-expr
    : primary-expr
    | postfix-expr postfix-op
    | postfix-expr '[' expression ']'
    | postfix-expr '{' expression '}'
    ;
*/
Expr* Parser::parsePostfixExpr()
{
    Expr* arg = parsePrimaryExpr();
    tok::Token op;
    Expr* rhs;
    while (true)
    {
        switch (lexer->Peek().type)
        {
        case tok::plusplus:
        case tok::minusminus:
            arg = new PostExpr(arg, lexer->Next());
            break;

        case listBegin:
            op = lexer->Next();
            rhs = parseExpression();
            if (!lexer->Expect(listEnd))
                err::ExpectedAfter(lexer, tok::Name(listEnd), "expression");
            else
                rhs->loc += lexer->Last().loc; //lump } in with length of arg. not great but good enough
            arg = new ListAccExpr(arg, rhs, op);
            break;

        case tupleBegin:
            op = lexer->Next();
            rhs = parseExpression();
            if (!lexer->Expect(tupleEnd))
                err::ExpectedAfter(lexer, tok::Name(tupleEnd), "expression");
            else
                rhs->loc += lexer->Last().loc;
            arg = new TupAccExpr(arg, rhs, op);
            break;

        default: //Toto, I've a feeling we're not in postfix anymore.
            return arg;
        }
    }
}

/*
primary-expr
    : IDENT | CONST | STRING_LITERAL
    | '{' '}' | '{' comma-expr '}'
    | '[' ']' | '[' comma-expr ']'
    | '*' type IDENT
    | '(' expression ')'
    ;
*/
Expr* Parser::parsePrimaryExpr()
{
    tok::Token to = lexer->Peek();

    Expr* ret;

    switch (to.type)
    {
    case tok::identifier:
        /*
        if (curScope->getTypeDef(to.value.int_v)) //it's a type
            return parseDecl();
        //nope, it's an ID
        */
        lexer->Advance();
        /*
        if (lexer->Peek() == tok::identifier) //but the user thinks its a type
        {
            err::Error(to.loc) << '\'' << cu->getIdent(to.value.int_v)
                << "' is not a type" << err::underline << err::endl; //we'll show them!
            to = lexer->Next(); //pretend it's a decl
        }
        */

        return new VarExpr(to.value.int_v, std::move(to.loc));
    case tok::integer:
        lexer->Advance();
        return new IntConstExpr(to.value.int_v, to.loc);
    case tok::floating:
        lexer->Advance();
        return new FloatConstExpr(to.value.dbl_v, to.loc);
    case tok::stringlit:
        lexer->Advance();
        return new StringConstExpr(to.value.int_v, to.loc);


    case tok::lparen:
        return parseBlock();

    case listBegin:
        lexer->Advance();
        ret = new ListifyExpr(parseExpression(), to);
        if (!lexer->Expect(listEnd, to))
            err::ExpectedAfter(lexer, tok::Name(listEnd), "expression list");
        else
            ret->loc += to.loc; //add } to expr location
        return ret;

    case tupleBegin:
        lexer->Advance();
        ret = new TuplifyExpr(parseExpression(), to);
        if (!lexer->Expect(tupleEnd, to))
            err::ExpectedAfter(lexer, tok::Name(tupleEnd), "expression list");
        else
            ret->loc += to.loc;
        return ret;

    case tok::star:
        lexer->Advance();
        if (couldBeType(lexer->Peek()))
            return parseDecl();
        else
        {
            err::ExpectedAfter(lexer, "type", "'*'");
            //not much we can do to recover here
            return new NullExpr(std::move(to.loc));
        }

    default:

        err::Error(to.loc) << "unexpected " << to.Name() << ", expecting expression"  << err::caret << err::endl;
        lexer->Advance(); //eat it
        return new NullExpr(std::move(to.loc));
    }
}
