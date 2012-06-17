#include "Type.h"
#include "Lexer.h"
#include "Util.h"
#include "Error.h"
#include "CompUnit.h"
#include "ParseUtils.h"
#include "Parser.h"

#include <cstdlib>

using namespace par;
using namespace typ;
using namespace ast;

bool par::couldBeType(tok::Token &t)
{
    switch (t.Type())
    {
    case listBegin:
    case tupleBegin:
    case tok::k_int:
    case tok::k_float:
    case tok::question:
    case tok::at:
    case tok::identifier:
    case tok::colon:
        return true;
    default:
        return false;
    }
}

void Parser::parseType()
{
    type.clear(); //reset type

    if (lexer->Expect(tok::colon)) //void function?
    {
        type.code += cod::any;
        type.code += cod::function;
        parseSingle();
        return;
    }

    parseSingle();
    parseTypeEnd();
}

void Parser::parseTypeEnd()
{
    if (lexer->Expect(tok::colon)) //function?
    {
        type.code += cod::function;
        parseSingle();
    }
}

/*
type
    : '{' type '}'
    | '[' type-list ']'
    | 'int' | 'float'
    | '?' | '?' IDENT
    | '@' type
    | IDENT
    ;
*/

void Parser::parseSingle()
{
    tok::Token to = lexer->Peek();
    switch (to.Type())
    {
    case listBegin:
        parseList();
        break;

    case tupleBegin:
        parseTuple();
        break;

    case tok::k_int:
    case tok::k_float:
        parsePrim();
        break;

    case tok::question:
        parseParam();
        break;

    case tok::at:
        parseRef();
        break;

    case tok::identifier:
        parseNamed();
        break;

    default:
        err::Error(to.loc) << "unexpected " << to.Name() << ", expecting type"
            << err::caret << err::endl;
        lexer->Advance();
    }
}

namespace par
{
    class TypeListParser
    {
        Parser *tp;
    public:
        int num;
        TypeListParser(Parser *t) : tp(t), num(0) {}
        void operator()(lex::Lexer *lexer)
        {
            ++num;
            tp->parseSingle();
            if (lexer->Peek() == tok::identifier)
            {
                tp->type.code += cod::tupname;
                tp->parseIdent();
            }
        }
    };
}

void Parser::parseTypeList()
{
    TypeListParser tlp(this);
    par::parseListOf(lexer, couldBeType, tlp, tupleEnd, "types");
}

void Parser::parseList()
{
    type.code += cod::list;
    lexer->Advance();
    parseSingle();
    parseListEnd();
}

void Parser::parseListEnd()
{
    if (!lexer->Expect(listEnd))
        err::ExpectedAfter(lexer, "'}'", "type");

    if (lexer->Expect(tok::bang))
    {
        tok::Token to;
        if (lexer->Expect(tok::integer, to))
        {
            type.code += cod::dim + utl::to_str(to.value.int_v);
        }
        else
            err::ExpectedAfter(lexer, "list length", "'!'");
    }
    type.code += cod::endOf(cod::list);
}

void Parser::parseTuple()
{
    type.code += cod::tuple;
    lexer->Advance();
    parseTypeList();
    parseTupleEnd();
}

void Parser::parseTupleEnd()
{
    //don't need to check for ], parseTypeList does that
    type.code += cod::endOf(cod::tuple);
}

void Parser::parsePrim()
{
    char c;

    if (lexer->Next() == tok::k_int)
        c = cod::integer;
    else 
        c = cod::floating;

    int width = 32;

    if (lexer->Expect(tok::bang))
    {
        tok::Token to;
        if (lexer->Expect(tok::integer, to))
        {
            width = (int)to.value.int_v;

            if (c == cod::integer)
            {
                if (width != 8 && width != 16 && width != 32 && width != 64)
                {
                    err::Error(to.loc) << '\'' << width << "' is not a valid integer width"
                        << err::caret << err::endl;
                    width = 32;
                }
            }
            else
            {
                if (width != 16 && width != 32 && width != 64 && width != 80)
                {
                    err::Error(to.loc) << '\'' << width << "' is not a valid floating point width"
                        << err::caret << err::endl;
                    width = 32;
                }
            }
        }
        else
            err::ExpectedAfter(lexer, "numeric width", "'!'");
    }

    type.code += c + utl::to_str(width) + cod::endOf(c);
}

void Parser::parseRef()
{
    type.code += cod::ref;
    lexer->Advance();
    parseSingle();
    type.code += cod::endOf(cod::ref);
}

void Parser::parseNamed()
{
	ast::TypeDef * td = curScope->getTypeDef(lexer->Peek().value.ident_v);

    if (!td)
    {
        err::Error(lexer->Peek().loc) << "undefined type '"
			<< lexer->getCompUnit()->getIdent(lexer->Peek().value.ident_v) << '\''
            << err::underline << err::endl;
        type.code += cod::integer + cod::endOf(cod::integer); //recover
        lexer->Advance(); //don't parse it twice
    }
    else
    {
        type.code += cod::named;
        parseIdent();
    }
    
    tok::Location argsLoc = lexer->Peek().loc;

    size_t nargs = checkArgs();

    argsLoc = argsLoc + lexer->Last().loc;

    if (td && nargs && nargs != td->params.size())
        err::Error(argsLoc) << "incorrect number of type arguments, expected 0 or " <<
            td->params.size() << ", got " << nargs << err::underline << err::endl;

    type.code += cod::endOf(cod::named);
}

void Parser::parseParam()
{
    lexer->Advance();
    if (lexer->Peek() == tok::identifier)
    {
        type.code += cod::param;
        parseIdent();
        type.code += cod::endOf(cod::param);
    }
    else
        type.code += cod::any + cod::endOf(cod::any);
}

size_t Parser::checkArgs()
{
    if (lexer->Expect(tok::bang))
    {
        type.code += cod::arg;
        if (lexer->Expect(tok::lparen))
        {
            TypeListParser tlp(this);
            par::parseListOf(lexer, couldBeType, tlp, tok::rparen, "types");
            type.code += cod::endOf(cod::arg);

            return tlp.num;
        }
        else
        {
            parseSingle();
            type.code += cod::endOf(cod::arg);
            
            return 1;
        }
    }
    return 0;
}

void Parser::parseIdent()
{
    type.code += utl::to_str(lexer->Next().value.int_v);
}
