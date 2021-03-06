#include "Parser.h"
#include "Lexer.h"
#include "Error.h"
#include "Module.h"
#include "Stmt.h"
#include "Global.h"
#include "Type.h"

using namespace par;
using namespace ast;

//big ole binary expression parser right here
//uses token precidence and associativity to parse it all in one function
Node0* Parser::parseExpression()
{
    Node0* lhs = parseUnaryExpr();
    return parseBinaryExprRHS(lhs, tok::prec::comma); //minumun prec is comma so ';' and stuff makes it return
}

/*
agg-expr
    : assignment-expr
    | AGG_OP agg-expr
    ;
*/
//needed because agg unary expr has lower precidence than assignment
Node0* Parser::parseBinaryExprInAgg()
{
    Node0* lhs = parseUnaryExpr();
    return parseBinaryExprRHS(lhs, tok::prec::assignment);
}

//TODO: don't other unary ops have precidence lower than binary ops?

Node0* Parser::parseBinaryExprRHS(Node0* lhs, tok::prec::precidence minPrec)
{
    while(lexer->Peek().Precidence() >= minPrec) //keep shifting?
    {
        tok::Token op = lexer->Next(); //we know its a bin op because others are prec::none

        Node0* rhs = parseUnaryExpr();

        //shift
        rhs = parseBinaryExprRHS(rhs, tok::prec::precidence(op.Precidence()
            + (op.Asso_ty() == tok::LeftAssoc ? 1 : 0)));
        //precidence can be the same if it is right associative

        //reduce

        VarExpr* func = dynamic_cast<VarExpr*>(lhs);
        if (op == tok::colon && func)
            lhs = new OverloadCallExpr(MkNPtr(func), Ptr(rhs), op.loc);
        else if (op == tok::equals)
            lhs = new AssignExpr(Ptr(lhs), Ptr(rhs), op.loc);
        else if (op == tok::opequals)
            lhs = new OpAssignExpr(Ptr(lhs), Ptr(rhs), op, curScope);
        else
            lhs = new OverloadCallExpr(op, curScope, Ptr(lhs), Ptr(rhs));

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
Node0* Parser::parseUnaryExpr()
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
        {
            lexer->Advance();
            Node0* arg = parsePostfixExpr();
            return new OverloadCallExpr(MkNPtr(new VarExpr(op, curScope)), Ptr(arg), arg->loc);
        }
    case tok::tick:
        lexer->Advance();
        return new IterExpr(Ptr(parsePostfixExpr()), op);
    case tok::opequals:
        lexer->Advance();
        return new AggExpr(Ptr(parseBinaryExprInAgg()), op);
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
Node0* Parser::parsePostfixExpr()
{
    Node0* arg = parsePrimaryExpr();
    tok::Token op;
    Node0* rhs;
    while (true)
    {
        switch (lexer->Peek().type)
        {
        case tok::plusplus:
        case tok::minusminus:
            arg = new PostExpr(Ptr(arg), lexer->Next());
            break;

        case listBegin:
            op = lexer->Next();
            rhs = parseExpression();
            if (!lexer->Expect(listEnd))
                err::ExpectedAfter(lexer, tok::Name(listEnd), "expression");
            else
                rhs->loc += lexer->Last().loc; //lump } in with length of arg. not great but good enough
            arg = new ListAccExpr(Ptr(arg), Ptr(rhs), op, curScope);
            break;

        case tupleBegin:
            op = lexer->Next();
            rhs = parseExpression();
            if (!lexer->Expect(tupleEnd))
                err::ExpectedAfter(lexer, tok::Name(tupleEnd), "expression");
            else
                rhs->loc += lexer->Last().loc;
            arg = new TupAccExpr(Ptr(arg), Ptr(rhs), op, curScope);
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
    | type IDENT
    | '(' expression ')'
    ;
*/
Node0* Parser::parsePrimaryExpr()
{
    tok::Token to = lexer->Peek();

    //backtracking works by setting a "restore point," then blindly parsing ahead as if it were
    //a type. if something goes wrong we set backtrackStatus accordingly and return without creating
    //any types. we then restore, and "try again" by parsing it as an expression instead.
    //TODO: maybe it would be better to only allow declarations at the beginning of expressions?
    //it would prevent the creation of lots of extraneous types
    //assume its a type first because there are many fewer places in code to error out and
    //need to backtrack from
    if (couldBeType(to))
    {
        //it must be a type. don't set backtracking in this case so we get better errors.
        if (to != tok::identifier && to != listBegin && to != tupleBegin)
            return parseDecl();

        lexer->backtrackSet(); //get ready...
        backtrackStatus = CanBacktrack; //get set...

        Node0* decl = parseDecl(); //GO!
        if (backtrackStatus != IsBacktracking) //success! it was a type.
        {
            backtrackStatus = CantBacktrack;
            return decl;
        }
        
        //nope! it was an expression
        backtrackStatus = CantBacktrack;
        lexer->backtrackReset();
        //now that we're back where we were, parse as an expression
    }

    switch (to.type)
    {
    case tok::identifier:
    {
        lexer->Advance();
        return new VarExpr(to.value.ident_v, curScope, to.loc);
    }
    {
        ConstExpr * ret;

    case tok::integer:
        lexer->Advance();
        ret = new ConstExpr(to.loc);
        ret->Annotate(typ::int64, val::Value(to.value.int_v));
        return ret;
    case tok::floating:
        lexer->Advance();
        ret = new ConstExpr(to.loc);
        //could use float!80, but use float!64 for compatibility
        ret->Annotate(typ::float64, val::Value(to.value.dbl_v));
        return ret;
    case tok::stringlit:
        lexer->Advance();
        ret = new ConstExpr(to.loc);
        ret->Annotate(Global().reserved.string_t, lexer->getCurVal());
        return ret;
    case tok::k_true:
    case tok::k_false:
        lexer->Advance();
        ret = new ConstExpr(to.loc);
        ret->Annotate(typ::boolean, val::Value(to == tok::k_true));
        return ret;
    }

    case tok::lparen:
        return parseBlock();
    case listBegin:
        return parseListify();
    case tupleBegin:
        return parseTuplify();

        //TODO: it might be better to try parsing it, then only eat it if we come back here
        //without advancing at all. as is, things like (1, ) or 5+; will mess up the parser because
        //it will ignore the ) and get all confused.
    default: //we're SOL
        err::Error(to.loc) << "unexpected " << to.Name() << ", expecting expression"  << err::caret;
        lexer->Advance(); //eat it so as not to loop forever
        Node0* ret = new NullExpr(to.loc);
        ret->Annotate(typ::error);
        return ret;
    }
}

/*
primary-expr
    : '{' expression '}'
    ;
*/
Node0* Parser::parseListify()
{
    lexer->Advance();

    Node0* ret = parseExpression();
    if (!lexer->Expect(listEnd))
        err::ExpectedAfter(lexer, "'}'", "expression list");
    return new ListifyExpr(Ptr(ret));
}

/*
primary-expr
    : '[' expression ']'
    ;
*/
Node0* Parser::parseTuplify()
{
    lexer->Advance();

    Node0* ret = parseExpression();
    if (!lexer->Expect(tupleEnd))
        err::ExpectedAfter(lexer, "']'", "expression list");
    return new TuplifyExpr(Ptr(ret));
}
