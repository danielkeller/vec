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
        err::Error(t.loc) << "redefinition of type '" << Global().getIdent(name) << '\'' << err::underline;
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

//TODO: should this be here?
//precondition: newDecl.Type() is actually a function type.
//this function will segfault otherwise
void OverloadGroupDeclExpr::Insert(FuncDeclExpr* newDecl)
{
    for (auto func : functions)
    {
        if (func->Type().getFunc().arg().compare(newDecl->Type().getFunc().arg())
            == typ::TypeCompareResult::valid)
        {
            if (func->Type().getFunc().ret().compare(newDecl->Type().getFunc().ret())
                == typ::TypeCompareResult::invalid)
            {
                err::Error(newDecl->loc) << "overloaded function differs only in return type"
                    << err::underline << func->loc << err::note << "see previous declaration"
                    << err::underline;
            }
            //let it know about the 'actual' decl
            newDecl->realDecl = func;
            return; //already declared
        }
    }

    //new!
    functions.push_back(newDecl);
}

/*
primary-expr
    : type IDENT
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
    DeclExpr* ret;


    if (type.getFunc().isValid())
    {
        //TODO: insert type params into this scope
        if (lexer->Peek() == tok::equals)
        {
            mod->scopes.emplace_back(curScope);
            curScope = &mod->scopes.back(); //create scope for function args
        }

        OverloadGroupDeclExpr* oGroup;
        if (DeclExpr* previousDecl = curScope->getVarDef(id))
        {
            oGroup = dynamic_cast<OverloadGroupDeclExpr*>(previousDecl);
            if (!oGroup)
            {
                err::Error(to.loc) << "redefinition of non-function as function" << err::underline
                    << previousDecl->loc << err::note << "see previous declaration" << err::underline;
                return new VarExpr(previousDecl, to.loc); //recover gracefully
            }
        }
        else //we have to create the overload group
        {
            oGroup = new OverloadGroupDeclExpr(id, to.loc);
            //overload groups are in the universal scope; the visibility of individual functions
            //is sorted out in sema 3. global also cleans them up this way!
            Global().universal.addVarDef(id, oGroup);
        }

        FuncDeclExpr* fde = new FuncDeclExpr(id, type, curScope, to.loc);

        oGroup->Insert(fde);

        //leave the decl expr hanging, it will get attached later
        //add argument definition to function's scope
        if (lexer->Peek() == tok::equals)
            curScope->addVarDef(Global().reserved.arg,
                new DeclExpr(Global().reserved.arg, type.getFunc().arg(), to.loc));

        return fde;
    }
    else //variable
    {
        if (DeclExpr* previousDecl = curScope->getVarDef(id))
        {
            //variables cannot be defined twice
            if (dynamic_cast<OverloadGroupDeclExpr*>(previousDecl))
            {
                err::Error(to.loc) << "redefinition of function as non-function" << err::underline
                    << previousDecl->loc << err::note << "see previous definition" << err::underline;
            }
            else
            {
                err::Error(to.loc) << "redefinition of variable '" << Global().getIdent(id)
                    << '\'' << err::underline;
            }

            return new VarExpr(curScope->getVarDef(id), to.loc); //recover gracefully
        }

        //this works because idents are given out sequentially
        if (id >= Global().reserved.opIdents.begin()->second && id <= Global().reserved.opIdents.rbegin()->second)
        {
            err::Error(to.loc) << "'operator' may only be used for functions" << err::underline;
            return new NullExpr(to.loc);
        }

        ret = new DeclExpr(id, type, to.loc);
        curScope->addVarDef(id, ret);
    }

    //this is an inconsistency in location tracking, we should have it start at
    //the beginning of the type, but we don't know where that is in this scope

    return ret;
}
