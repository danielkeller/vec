#include "Type.h"
#include "Lexer.h"
#include "Util.h"
#include "Error.h"
#include "Module.h"
#include "Parser.h"
#include "Global.h"

#include <cstdlib>

using namespace par;
using namespace typ;
using namespace ast;

#define START_BACKTRACK \
do {\
    if (backtrackStatus == CanBacktrack)\
    {\
        backtrackStatus = IsBacktracking;\
        type = typ::error;\
        return;\
    }\
} while (false)

bool par::couldBeType(tok::Token &t)
{
    switch (t.Type())
    {
    case listBegin:
    case tupleBegin:
    case tok::k_int:
    case tok::k_float:
    case tok::k_bool:
    case tok::question:
    case tok::at:
    case tok::identifier:
    case tok::colon:
        return true;
    default:
        return false;
    }
}

/*
type
    : single-type
    | ':' single-type
    | single-type ':' single-type
    ;
*/
void Parser::parseType()
{
    if (lexer->Peek() == tok::colon) //void function?
        type = typ::null;
    else
        parseSingle();

    if (backtrackStatus != IsBacktracking)
        parseTypeEnd();
}

void Parser::parseTypeEnd()
{
    if (lexer->Expect(tok::colon)) //function?
    {
        Type ret = type;
        parseSingle();
        if (backtrackStatus != IsBacktracking)
            type = typ::mgr.makeFunc(ret, type);
    }
}

/*
single-type
    : '{' type-list '}'
    | '[' type ']'
    | ('int' | 'float') ('!' int-const)?
    | '?' | '?' IDENT
    | '@' type
    | IDENT | IDENT '!' type | IDENT '!' '(' type-list ')'
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
    case tok::k_bool:
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
        START_BACKTRACK;
        err::Error(to.loc) << "unexpected " << to.Name() << ", expecting type" << err::caret;
        type = typ::error;
        lexer->Advance();
    }
}



/*
single-type
    : '[' type ']'
    ;
*/
void Parser::parseList()
{
    lexer->Advance();
    parseSingle();
    if (backtrackStatus == IsBacktracking)
        return;

    if (!lexer->Expect(listEnd))
    {
        //could be a declaration or something
        START_BACKTRACK;
        err::ExpectedAfter(lexer, "']'", "type");
    }

    if (lexer->Expect(tok::bang))
    {
        tok::Token to;
        if (lexer->Expect(tok::integer, to))
        {
            type = typ::mgr.makeList(type, to.value.ident_v);
        }
        else //don't backtrack?
        {
            err::ExpectedAfter(lexer, "list length", "'!'");
            type = typ::mgr.makeList(type);
        }
    }
    else
        type = typ::mgr.makeList(type);
}

/*
single-type
    : '{' type-list '}'
    ;
type-list
    : type
    | type-list ',' type
    ;
*/
void Parser::parseTuple()
{
    typ::TupleBuilder builder;

    lexer->Advance();
    do {
        parseSingle();

        if (backtrackStatus == IsBacktracking)
            return;
         
        if (lexer->Peek() == tok::identifier)
        {
            builder.push_back(type, lexer->Next().value.ident_v);
        }
        else
            builder.push_back(type, Global().reserved.null);
    } while (lexer->Expect(tok::comma));

    if (!lexer->Expect(tupleEnd))
    {
        START_BACKTRACK;
        err::ExpectedAfter(lexer, "'}'", "type list");
    }

    type = typ::mgr.makeTuple(builder);
}

/*
single-type
    : ('int' | 'float') ('!' int-const)?
    ;
*/
void Parser::parsePrim()
{
    if (lexer->Expect(tok::k_int))
    {
        if (!lexer->Expect(tok::bang))
        {
            type = typ::int32;
            return;
        }

        tok::Token to;
        if (!lexer->Expect(tok::integer, to))
        {
            err::ExpectedAfter(lexer, "numeric width", "'!'");
            type = typ::int32;
            return;
        }

        switch (to.value.int_v)
        {
        case 8:
            type = typ::int8;
            break;
        case 16:
            type = typ::int16;
            break;
        case 32:
            type = typ::int32;
            break;
        case 64:
            type = typ::int64;
            break;
        default:
            err::Error(to.loc) << '\'' << to.value.int_v << "' is not a valid integer width"
                << err::underline;
            type = typ::int32;
            break;
        }
    }
    else if (lexer->Expect(tok::k_float))
    {
        if (!lexer->Expect(tok::bang))
        {
            type = typ::float32;
            return;
        }

        tok::Token to;
        if (!lexer->Expect(tok::integer, to))
        {
            err::ExpectedAfter(lexer, "numeric width", "'!'");
            type = typ::float32;
            return;
        }

        switch (to.value.int_v)
        {
            case 32:
                type = typ::float32;
                break;
            case 64:
                type = typ::float64;
                break;
            case 80:
                type = typ::float80;
                break;
            default:
                err::Error(to.loc) << '\'' << to.value.int_v << "' is not a valid floating point width"
                    << err::underline;
                type = typ::float32;
                break;
        }
    }
    else //bool
    {
        lexer->Advance();
        type = typ::boolean;
    }
}

/*
single-type
    : '@' type
    ;
*/
void Parser::parseRef()
{
    lexer->Advance();
    parseSingle();
    if (backtrackStatus != IsBacktracking)
        type = typ::mgr.makeRef(type);
}

/*
single-type
    : IDENT
    | IDENT '!' type
    | IDENT '!' '(' type-list ')'
    ;
*/
void Parser::parseNamed()
{
    tok::Token id = lexer->Next();

    Ident typeName = id.value.ident_v;
    ast::TypeDef * td = curScope->getTypeDef(typeName);

    tok::Location argsLoc = lexer->Peek().loc;

    typ::NamedBuilder builder;
    unsigned int nargs = 0;

    if (lexer->Expect(tok::bang))
    {
        //this is incorrect for an expression, disable backtracking
        backtrackStatus = CantBacktrack;

        if (lexer->Expect(tok::lparen))
        {
            do {
                parseSingle();
                if (backtrackStatus == IsBacktracking)
                    return;

                builder.push_back(type);
                ++nargs;
            } while (lexer->Expect(tok::comma));
            if (!lexer->Expect(tok::rparen))
                err::ExpectedAfter(lexer, ")", "list of types");
        }
        else
        {
            parseSingle();
            if (backtrackStatus == IsBacktracking)
                return;

            builder.push_back(type);
            nargs = 1;
        }
    }

    argsLoc = argsLoc + lexer->Last().loc;

    //no type was found, it could be defined in another package, so save it off for
    //importing to worry about
    if (!td)
    {
        type = typ::mgr.makeExternNamed(typeName, builder);

        //only complain about the first instance. we only need one anyway to fix the type. 
        //if the previous one could be extraneous but this one can't, replace it for better 
        //errors

        if (!mod->externTypes.count(type))
            mod->externTypes[type] = Module::ExternTypeInfo(id.loc, argsLoc);
        return;
    }
    //otherwise, deal with it now

    if (nargs && nargs != td->params.size())
        err::Error(argsLoc) << "incorrect number of type arguments, expected 0 or " <<
            td->params.size() << ", got " << nargs << err::underline;

    //don't worry about recovering, aliasing fixes this for us

    type = typ::mgr.makeNamed(td->mapped, typeName, td->params, builder);
}

/*
single-type
    : '?'
    | '?' IDENT
    ;
*/
void Parser::parseParam()
{
    lexer->Advance();
    if (lexer->Peek() == tok::identifier)
    {
        type = typ::mgr.makeParam(lexer->Next().value.ident_v);
    }
    else
        type = typ::any;
}
