#include "Parser.h"
#include "Lexer.h"
#include "Error.h"
#include "Type.h"
#include "Module.h"
#include "Global.h"

#include <cassert>

using namespace par;
using namespace ast;

Parser::Parser(lex::Lexer *l)
    : lexer(l),
    mod(l->getModule()),
    backtrackStatus(CantBacktrack)
{
    curScope = &mod->pub;

    if (lexer->Expect(tok::k_module))
    {
        tok::Token name;
        if (lexer->Expect(tok::identifier, name))
            mod->name = Global().getIdent(name.value.ident_v);
        else
            err::ExpectedAfter(lexer, "identifier", "'module'");

        if (!lexer->Expect(tok::semicolon))
            err::ExpectedAfter(lexer, ";", "module definition");
    }

    mod->setChildA(Ptr(parseStmtList()));

    tok::Token& end = lexer->Peek();
    if (end != tok::end)
        err::Error(end.loc) << "unexpected '" << end.Name() << "', expecting EOF" << err::caret;
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
        err::Error(t.loc) << "redefinition of type '" << name << '\'' << err::underline;
        lexer->ErrUntil(tok::semicolon);
        return;
    }

    if (lexer->Expect(tok::question)) //has parameters
    {
        if (lexer->Expect(tok::lparen)) //list
        {
            do {
                if (lexer->Peek() == tok::identifier)
                {
                    if (std::count(td.params.begin(), td.params.end(), lexer->Peek().value.ident_v))
                        err::Error(lexer->Peek().loc) << "type parameters must be unique" << err::underline;
                    td.params.push_back(lexer->Next().value.ident_v);
                }
                else
                    err::Error(lexer->Peek().loc) << "expected identifier in type parameter list"
                        << err::underline;
            } while (lexer->Expect(tok::comma));
            if (!lexer->Expect(tok::rparen))
                err::ExpectedAfter(lexer, ")", "list of identifers");
        }
        else if (lexer->Peek() == tok::identifier) //single
            td.params.push_back(lexer->Next().value.ident_v);
        else //junk
        {
            err::Error(lexer->Peek().loc) << "unexpected " << lexer->Peek().Name()
                << " expected type parameter list" << err::caret;
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
Node0* Parser::parseDecl()
{
    parseType();
    
    if (backtrackStatus == IsBacktracking)
        return 0;

    return parseDeclRHS();
}

/*
primary-expr
    : type IDENT
    | type IDENT ('{' (IDENT ',')* IDENT '}') | '{' '}' expr
    ;
*/
Node0* Parser::parseDeclRHS()
{
    tok::Token to;

    if (!lexer->Expect(tok::identifier, to))
    {
        if (backtrackStatus == CanBacktrack)
        {
            backtrackStatus = IsBacktracking;
            return 0; //this value will get ignored anyway
        }

        err::Error(lexer->Last().loc) << "expected identifier in declaration" << err::postcaret;
        return new NullExpr(lexer->Last().loc);
    }

    Ident id = to.value.ident_v;
    Node0* ret = new DeclExpr(id, curScope, type, to.loc);

    //There are no redefinitions, because there are no definitions, only assignment.
    //Redeclarations are fine. In case of values declared with multiple types, Exec
    //can issue an error/warning

    if (lexer->Expect(tupleBegin))
    {
        backtrackStatus = CantBacktrack; //there's nothing else it could be
        std::vector<Ident> params;
        while (true)
        {
            tok::Token pTo;
            if (!lexer->Expect(tok::identifier, pTo))
            {
                err::Error(lexer->Last().loc) << "expected identifier" << err::postcaret;
                break;
            }
            params.push_back(pTo.value.ident_v);

            if (!lexer->Expect(tok::comma))
            {
                if (!lexer->Expect(tupleEnd))
                    err::ExpectedAfter(lexer, "'}'", "parameter list");
                break;
            }
        }

        NormalScope* oldScope = curScope;
        mod->scopes.emplace_back(curScope);
        curScope = &mod->scopes.back();

        Ptr conts = Ptr(parseExpression());

        for (auto p : params)
        {
            DeclExpr* pdecl = new DeclExpr(p, curScope, typ::error, ret->loc);
            curScope->addVarDef(pdecl);
            conts = Ptr(new StmtPair(Ptr(pdecl), move(conts)));
        }

        Lambda* body = new Lambda(id, move(params), curScope, move(conts));
        //FIXME: this is a hack until type merging works
        body->Annotate(ret->Type());

        ret = new AssignExpr(Ptr(ret), Ptr(body), to.loc);
        curScope = oldScope;
    }

    return ret;
}
