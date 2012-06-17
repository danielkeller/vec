#include "Parser.h"
#include "Lexer.h"
#include "Error.h"
#include "Type.h"
#include "CompUnit.h"
#include "ParseUtils.h"

#include <cassert>

using namespace par;
using namespace ast;

Parser::Parser(lex::Lexer *l)
    : lexer(l),
    cu(l->getCompUnit())
{
    curScope = &cu->global;

    cu->treeHead = parseStmtList();
}

namespace
{
bool isIdent(tok::Token &t)
{
    return t == tok::identifier;
}
class TypeDeclParamParser
{
    TypeDef *td;
public:
    TypeDeclParamParser(TypeDef *d) : td(d) {}
    void operator()(lex::Lexer *l)
    {
        td->params.push_back(l->Next().value.ident_v);
    }
};
}

/*
type-decl
    : 'type' IDENT = type ';'
    | 'type' IDENT '?' IDENT = type ';'
    | 'type' IDENT '?' '(' ident-list ')' = type ';'
    ;
*/
void Parser::parseTypeDecl()
{
    TypeDef td;
    tok::Token t;
    lexer->Advance(); //eat 'type'

    if (!lexer->Expect(tok::identifier, t))
    {
        err::ExpectedAfter(lexer, "identifier", "'type'");
        lexer->ErrUntil(tok::semicolon);
        return;
    }

    Ident name = t.value.ident_v;

    if (curScope->getTypeDef(name))
    {
        err::Error(t.loc) << "redefinition of type '" << cu->getIdent(name) << '\''
        << err::underline << err::endl;
        lexer->ErrUntil(tok::semicolon);
        return;
    }

    if (lexer->Expect(tok::question)) //has parameters
    {
        if (lexer->Expect(tok::lparen)) //list
        {
            TypeDeclParamParser tdp(&td);
            par::parseListOf(lexer, isIdent, tdp, tok::rparen, "identifiers");
        }
        else if (lexer->Peek() == tok::identifier) //single
            td.params.push_back(lexer->Next().value.ident_v);
        else //junk
        {
            err::Error(lexer->Peek().loc) << "unexpected " << lexer->Peek().Name()
            << " in type parameter list" << err::caret << err::endl;
            lexer->ErrUntil(tok::semicolon);
            return;
        }
    }

    if (!lexer->Expect(tok::equals))
        err::ExpectedAfter(lexer, "'='", "type name");

    parseType(); //create type in t
    td.mapped = type;

    if (!lexer->Expect(tok::semicolon))
        err::ExpectedAfter(lexer, "';'", "type definition");

    curScope->addTypeDef(name, td);
}

/*
primary-expr
    : type IDENT
    ;
*/
Expr* Parser::parseDecl()
{
    parseType();

    return parseDeclRHS();
}

/*
primary-expr
    : type IDENT
    ;
*/
Expr* Parser::parseDeclRHS()
{
    tok::Token to;

    if (!lexer->Expect(tok::identifier, to))
    {
        err::Error(lexer->Last().loc) << "expected identifier in declaration" << err::postcaret << err::endl;
        return new NullExpr(std::move(lexer->Last().loc));
    }

    Ident id = to.value.ident_v;

    if (curScope->getVarDef(id))
    {
        //variables cannot be defined twice
        if (!type.isFunc())
            err::Error(to.loc) << "redefinition of variable '" << cu->getIdent(id)
            << '\'' << err::underline << err::endl;
    }
    else
    {
        VarDef vd;
        vd.type = type;
        curScope->addVarDef(id, vd);
    }

    //the following is an inconsistency in location tracking, we should have it start at
    //the beginning of the type, but we don't know where that is in this scope
    return new VarExpr(id, std::move(to.loc));
}
