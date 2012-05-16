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
    while (lexer->Peek() != tok::end)
        parseGlobalDecl();
}

void Parser::parseGlobalDecl()
{
    tok::Token t = lexer->Peek();

    if (t == tok::k_type)
        parseTypeDecl();
    else if (typ::couldBeType(t))
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
    tok::Token t = lexer->Next();

    if (t != tok::identifier)
    {
        err::Error(typeloc) << "expected identifier after type" << err::postcaret << err::endl;
        lexer->ErrUntil(tok::semicolon);
        return;
    }
    
    td.name = t.value.int_v;

    if (cu->getTypeDef(td.name))
    {
        err::Error(t.loc) << "redefinition of type '" << cu->getIdent(td.name) << '\''
            << err::underline << err::endl;
        lexer->ErrUntil(tok::semicolon);
        return;
    }

    if (lexer->Expect(tok::question)) //has parameters
    {
        if (lexer->Expect(typ::tupleBegin)) //list
        {
            TypeDeclParamParser tdp(&td);
            par::parseListOf(lexer, isIdent, tdp, typ::tupleEnd, "identifiers");
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
        err::Error(lexer->Peek().loc) << "expected = after type name" << err::caret << err::endl;

    td.mapped = typ::Type(lexer); //create type

    if (!lexer->Expect(tok::semicolon))
        err::Error(lexer->Last().loc) << "expected ; after type declaration" << err::postcaret << err::endl;

    cu->addTypeDef(td);
    std::cerr << td.mapped.w_str() << " = " << td.mapped.ex_w_str() << std::endl;
}

void Parser::parseDecl()
{
    typ::Type t(lexer);
    std::cerr << t.w_str() << " = " << t.ex_w_str() << std::endl;
}
