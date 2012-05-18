#include "Parser.h"
#include "Lexer.h"
#include "Error.h"
#include "Type.h"
#include "CompUnit.h"
#include "ParseUtils.h"

using namespace par;

Parser::Parser(lex::Lexer *l)
    : lexer(l), cu(l->getCompUnit())
{
    curScope = cu->globalScope();

    while (lexer->Peek() != tok::end)
        parseGlobalDecl();
}

void Parser::parseGlobalDecl()
{
    tok::Token t = lexer->Peek();

    if (t == tok::k_type)
        parseTypeDecl();
    else if (couldBeType(t))
        parseDecl();
    else
    {
        err::Error(t.loc) << "unexpected " << t.Name() <<
        " at global level. did you forget a { ?" << err::caret << err::endl;

        //recover:
        //eat tokens until we find one more } than {
        int braceLvl = 1;
        while (braceLvl != 0 && lexer->Peek() != tok::end)
        {
            switch (lexer->Next().type)
            {
            case tok::lbrace:
                braceLvl++;
                break;
            case tok::rbrace:
                braceLvl--;
                break;
            default:
                break;
            }
        }
    }
}

namespace
{
    bool isIdent(tok::Token &t) {return t == tok::identifier;}
    class TypeDeclParamParser
    {
        ast::TypeDef *td;
    public:
        TypeDeclParamParser(ast::TypeDef *d) : td(d) {}
        void operator()(lex::Lexer *l)
        {
            td->params.push_back(l->Next().value.int_v);
        }
    };
}

void Parser::parseTypeDecl()
{
    tok::Location typeloc = lexer->Next().loc;
    ast::TypeDef td;
    tok::Token t;

    if (!lexer->Expect(tok::identifier, t))
    {
        err::ExpectedAfter(lexer, "identifier", "'type'");
        lexer->ErrUntil(tok::semicolon);
        return;
    }
    
    ast::Ident name = t.value.int_v;

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
            td.params.push_back(lexer->Next().value.int_v);
        else //junk
        {
            err::Error(lexer->Peek().loc) << "unexpected " << lexer->Peek().Name()
                << " in type parameter list" << err::caret << err::endl;
            lexer->ErrUntil(tok::semicolon);
            return;
        }
    }

    if (!lexer->Expect(tok::equals))
        err::ExpectedAfter(lexer, "=", "type name");

    td.mapped = tp(lexer, curScope); //create type

    if (!lexer->Expect(tok::semicolon))
        err::ExpectedAfter(lexer, ";", "type declaration");

    curScope->addTypeDef(name, td);
}

void Parser::parseDecl()
{
    typ::Type t = tp(lexer, curScope);

    tok::Token to;

    if (!lexer->Expect(tok::identifier, to))
    {
        err::ExpectedAfter(lexer, "identifier", "type");
        lexer->ErrUntil(tok::semicolon);
        return;
    }

    ast::Ident id = to.value.int_v;
    
    if (!t.isFunc()) //variable definition
    {
        //variables cannot be defined twice
        if (curScope->getVarDef(id))
        {
            err::Error(to.loc) << "redefinition of variable '" << cu->getIdent(id)
                << '\'' << err::underline << err::endl;
            lexer->ErrUntil(tok::semicolon);
            return;
        }

        ast::VarDef vd;
        vd.type = t;
        curScope->addVarDef(id, vd);
    }

    if (!lexer->Expect(tok::semicolon))
        err::ExpectedAfter(lexer, ";", "variable definition");
}
