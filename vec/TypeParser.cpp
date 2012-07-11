#include "Type.h"
#include "Lexer.h"
#include "Util.h"
#include "Error.h"
#include "CompUnit.h"
#include "Parser.h"

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
            type = cu->tm.makeFunc(ret, type);
    }
}

/*
single-type
    : '{' type '}'
    | '[' type-list ']'
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
    : '{' type '}'
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
        err::ExpectedAfter(lexer, "'}'", "type");
    }

    if (lexer->Expect(tok::bang))
    {
        tok::Token to;
        if (lexer->Expect(tok::integer, to))
        {
            type = cu->tm.makeList(type, to.value.ident_v);
        }
        else //don't backtrack?
        {
            err::ExpectedAfter(lexer, "list length", "'!'");
            type = cu->tm.makeList(type);
        }
    }
    else
        type = cu->tm.makeList(type);
}

/*
single-type
    : '[' type-list ']'
    ;
type-list
    : type
    | type-list ',' type
    ;
*/
void Parser::parseTuple()
{
    TupleRAII raiiobj(cu->tm);

    lexer->Advance();
    do {
        parseSingle();

        if (backtrackStatus == IsBacktracking)
            return;
         
        if (lexer->Peek() == tok::identifier)
        {
            cu->tm.addToTuple(type, lexer->Next().value.ident_v);
        }
        else
            cu->tm.addToTuple(type, cu->reserved.null);
    } while (lexer->Expect(tok::comma));

    if (!lexer->Expect(tupleEnd))
    {
        START_BACKTRACK;
        err::ExpectedAfter(lexer, tok::Name(tupleEnd), "type list");
    }

    type = cu->tm.finishTuple();
}

/*
single-type
    : ('int' | 'float') ('!' int-const)?
    ;
*/
void Parser::parsePrim()
{
    if (lexer->Next() == tok::k_int)
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
    else 
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
           case 16:
                type = typ::float16;
                break;
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
        type = cu->tm.makeRef(type);
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

    ast::Ident typeName = id.value.ident_v;
    ast::TypeDef * td = curScope->getTypeDef(typeName);

    if (!td)
    {
        //err::Error(id.loc) << "undefined type '"
        //    << lexer->getCompUnit()->getIdent(typeName) << '\'' << err::underline;
        type = typ::int32; //recover
        return;
    }

    tok::Location argsLoc = lexer->Peek().loc;

    size_t nargs = 0;

    if (lexer->Expect(tok::bang))
    {
        if (lexer->Expect(tok::lparen))
        {
            do {
                parseSingle();
                if (backtrackStatus == IsBacktracking)
                {
                    cu->tm.clearNamedArgs();
                    return;
                }

                if (nargs < td->params.size()) //otherwise, error
                    cu->tm.addNamedArg(td->params[nargs], type);
                ++nargs; //keep incrementing, we can detect the error that way
            } while (lexer->Expect(tok::comma));
            if (!lexer->Expect(tok::rparen))
                err::ExpectedAfter(lexer, ")", "list of types");
        }
        else
        {
            parseSingle();
            if (backtrackStatus == IsBacktracking)
                return;

            if (td->params.size() == 1) //otherwise, error
                cu->tm.addNamedArg(td->params[0], type);
            nargs = 1;
        }
    }

    argsLoc = argsLoc + lexer->Last().loc;

    if (td && nargs && nargs != td->params.size())
        err::Error(argsLoc) << "incorrect number of type arguments, expected 0 or " <<
            td->params.size() << ", got " << nargs << err::underline;

    //recover as best we can. also do "aliasing" (ie replacing ?T with ? when not spec'd) here.
    //FIXME: Thats not aliasing. For example, cosider
    // type foo = [?T, ?T];
    // type bar = foo;
    // bar would become [?, ?] which is wrong.
    // possibly get the name of the param and replace '?T' with '?T in foo'
    while (nargs < td->params.size())
        cu->tm.addNamedArg(td->params[nargs++], typ::any);

    type = cu->tm.finishNamed(td->mapped, typeName);
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
        type = cu->tm.makeParam(lexer->Next().value.ident_v);
    }
    else
        type = typ::any;
}
