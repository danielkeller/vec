#include "Parser.h"
#include "Lexer.h"
#include "Error.h"
#include "CompUnit.h"
#include "Stmt.h"

using namespace par;
using namespace ast;

//big ole binary expression parser right here
//uses token precidence and associativity to parse it all in one function
Expr* Parser::parseBinaryExpr()
{
    Expr* lhs = parseUnaryExpr();
    return parseBinaryExprRHS(lhs, tok::prec::comma); //minumun prec is comma so ';' and stuff makes it return
}

Expr* Parser::parseBinaryExprRHS(Expr* lhs, tok::prec::precidence minPrec)
{
    while(true)
    {
        if (lexer->Peek().Precidence() < minPrec)
            return lhs; //cave johnson, we're done here

        tok::Token op = lexer->Next(); //we know its a bin op because others are prec::none

        Expr* rhs = parseUnaryExpr();
        tok::Token &tNext = lexer->Peek();

        if (tNext.Precidence() > op.Precidence()
            || (tNext.Precidence() == op.Precidence() && op.Asso_ty() == tok::RightAssoc)) //shift
        {
            rhs = parseBinaryExprRHS(rhs, tok::prec::precidence(op.Precidence()
                + (op.Asso_ty() == tok::LeftAssoc ? 1 : 0)));
            //precidence can be the same if it is right associative
        }
        //reduce
        err::Error(lhs->loc) << "wheee expression" << err::underline << op.loc << err::caret
            << rhs->loc << err::underline << err::endl;
        lhs = makeBinExpr(lhs, rhs, op);
        //fall through and "tail recurse"
    }
}

Expr* Parser::parseUnaryExpr()
{
    tok::Token tmp;
    if (!lexer->Expect(tok::identifier, tmp))
        std::cerr << "oh noes expected identifier" << std::endl;
    return new VarExpr(tmp.value.int_v, std::move(tmp.loc));
}
