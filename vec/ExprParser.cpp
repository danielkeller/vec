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

        //special case for functions
        if (op == tok::equals && dynamic_cast<VarExpr*>(lhs))
        {
            VarExpr* ve = dynamic_cast<VarExpr*>(lhs);
            if (ve->type.isFunc())
            {
                rhs = new Block(new ExprStmt(rhs), curScope, std::move(rhs->loc)); //wrap in block with function scope created in DeclParser
                curScope = curScope->getParent(); //pop scope stack
            }
        }

        //reduce
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
    | type IDENT
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
		if (curScope->getTypeDef(to.value.ident_v)) //it's a type
            return parseDecl();
        //nope, it's an ID
        lexer->Advance();
        if (lexer->Peek() == tok::identifier) //but the user thinks its a type
        {
            err::Error(to.loc) << '\'' << cu->getIdent(to.value.ident_v)
                << "' is not a type" << err::underline << err::endl; //we'll show them!
            to = lexer->Next(); //pretend it's a decl
        }
        return new VarExpr(to.value.ident_v, std::move(to.loc));

    case tok::integer:
        lexer->Advance();
        return new IntConstExpr(to.value.int_v, to.loc);
    case tok::floating:
        lexer->Advance();
        return new FloatConstExpr(to.value.dbl_v, to.loc);
    case tok::stringlit:
        lexer->Advance();
        return new StringConstExpr(to.value.ident_v, to.loc);


    case tok::lparen:
        return parseBlock();

    case listBegin:
        type.clear(); //-orIfy doesn't clear type so do it here
        ret = parseListOrIfy();
        if (!ret) //it's a type
        {
            parseTypeEnd(); //must be here because here we know it's a top level type
            ret = parseDeclRHS();
        }
        return ret;

    case tupleBegin:
        type.clear(); //-orIfy doesn't clear type so do it here
        ret = parseTupleOrIfy();
        if (!ret) //it's a type
        {
            parseTypeEnd(); //must be here because here we know it's a top level type
            ret = parseDeclRHS();
        }
        return ret;

    default:
        if (couldBeType(lexer->Peek())) //any other type stuff
            return parseDecl();

        //otherwise we're SOL

        err::Error(to.loc) << "unexpected " << to.Name() << ", expecting expression"  << err::caret << err::endl;
        lexer->Advance(); //eat it
        return new NullExpr(std::move(to.loc));
    }
}

/*
type
    : '{' type '}'
    ;

primary-expr
    : '{' expression '}'
    ;
*/
Expr* Parser::parseListOrIfy()
{
    //go ahead and insert into the type, if we're wrong it'll get cleared anyway
    type.code += typ::cod::list;
    tok::Token brace = lexer->Next();

    //now see what's inside it...
    tok::Token to = lexer->Peek();
    if (to == tok::identifier) //ambiguous case #1
    {
        if (curScope->getTypeDef(to.value.ident_v)) //it's a type
        {
            parseNamed();
            parseListEnd();
            return NULL;
        }

        //it's an expr
        Expr* ret = parseExpression();
        if (!lexer->Expect(listEnd))
            err::ExpectedAfter(lexer, "'}'", "expression list");
        return new ListifyExpr(ret, brace);
    }
    else if (to == listBegin || to == tupleBegin) //ambiguous case #2
    {
        Expr* ret;
        if (to == listBegin)
            ret = parseListOrIfy();
        else
            ret = parseTupleOrIfy();

        if (ret) //it's an expr
        {
            if (!lexer->Expect(listEnd))
                err::ExpectedAfter(lexer, "'}'", "expression list");
            return new ListifyExpr(ret, brace);
        }

        //it's a type
        parseListEnd();
        return NULL;
    }
    else if (couldBeType(to)) //otherwise, if it looks like a type, it is a type
    {
        parseSingle();
        parseListEnd();
        return NULL;
    }

    //otherwise, it's an expression
    Expr* ret = parseExpression();
    if (!lexer->Expect(listEnd))
        err::ExpectedAfter(lexer, "'}'", "expression list");
    return new ListifyExpr(ret, brace);
}

/*
type
    : '[' type ']'
    ;

primary-expr
    : '[' expression ']'
    ;
*/
Expr* Parser::parseTupleOrIfy()
{
    //go ahead and insert into the type, if we're wrong it'll get cleared anyway
    type.code += typ::cod::tuple;
    tok::Token brace = lexer->Next();

    //now see what's inside it...
    tok::Token to = lexer->Peek();
    if (to == tok::identifier) //ambiguous case #1
    {
        if (curScope->getTypeDef(to.value.ident_v)) //it's a type
        {
            lexer->Expect(tok::comma); //get rid of the comma if it's there
            parseTypeList();
            parseTupleEnd();
            return NULL;
        }

        //it's an expr
        Expr* ret = parseExpression();
        if (!lexer->Expect(tupleEnd))
            err::ExpectedAfter(lexer, "']'", "expression list");
        return new TuplifyExpr(ret, brace);
    }
    else if (to == listBegin || to == tupleBegin) //ambiguous case #2
    {
        Expr* ret;
        if (to == listBegin)
            ret = parseListOrIfy();
        else
            ret = parseTupleOrIfy();

        if (ret) //it's an expr
        {
            if (!lexer->Expect(tupleEnd))
                err::ExpectedAfter(lexer, "']'", "expression list");
            return new TuplifyExpr(ret, brace);
        }

        //it's a type
        //there might be more types to parse
        //or a tuple element name
        if (lexer->Peek() == tok::identifier)
        {
            type.code += typ::cod::tupname;
            parseIdent();
        }
        lexer->Expect(tok::comma); //get rid of the comma if it's there
        parseTypeList();
        parseTupleEnd();
        return NULL;
    }
    else if (couldBeType(to)) //otherwise, if it looks like a type, it is a type
    {
        lexer->Expect(tok::comma); //get rid of the comma if it's there
        parseTypeList();
        parseTupleEnd();
        return NULL;
    }
    //otherwise, it's an expression
    Expr* ret = parseExpression();
    if (!lexer->Expect(tupleEnd))
        err::ExpectedAfter(lexer, "']'", "expression list");
    return new TuplifyExpr(ret, brace);
}
