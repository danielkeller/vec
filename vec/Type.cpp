#include "Type.h"
#include "Lexer.h"
#include "Util.h"
#include "Error.h"
#include "CompUnit.h"
#include "ParseUtils.h"

#include <stack>
#include <cstdlib>

using namespace typ;

namespace cod
{
    static const char list = 'L';
    static const char tuple = 'T';
    static const char tupname = 'v'; //name of variable for struct-style access
    static const char arg = 'A';
    static const char integer = 'I';
    static const char floating = 'F';
    static const char ref = 'R';
    static const char named = 'N';
    static const char any = 'Y';
    static const char param = 'P';
    static const char function = 'U';

    char endOf(const char begin)
    {
        return begin - 'A' + 'a'; //ie lowercase version
    }
}

bool typ::couldBeType(tok::Token &t)
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
        return true;
    default:
        return false;
    }
}

namespace
{
    int readNum(std::string::iterator &it)
    {
        char *end;
        int ret = strtol(&*it, &end, 10);
        it = std::string::iterator(end);
        return ret;
    }

    std::string readType(std::string::iterator &it)
    {
        std::string::iterator start = it;
        char begin = *it;
        char end = cod::endOf(*it);
        int lvl = 1;
        ++it;
        while (lvl > 0)
        {
            if (*it == begin)
                ++lvl;
            else if (*it == end)
                --lvl;
            ++it;
        }
        return std::string(start, it);
    }   

    std::pair<int, int> readNumAndOwner(std::string::iterator &it)
    {
        char *end;

        int alias = strtol(&*it, &end, 10);
        it = std::string::iterator(end);

        return std::make_pair(alias & 0xFFFF, (alias >> 16) - 1);
    }
}

Type::Type()
    : code("I"), expanded("I")
{}

Type::Type(lex::Lexer *l)
{
    parseSingle(l);
    if (l->Expect(tok::colon)) //function?
    {
        code += cod::function;
        parseSingle(l);
    }

    //fill in expnaded type
    expanded.clear();
    std::string::iterator it = code.begin();

    while (it != code.end())
    {
        if (*it == cod::named) //fill in named types
        {
            ++it;
            ast::Ident id = readNum(it);
            ast::TypeDef *td = l->getCompUnit()->getTypeDef(id);

            std::map<ast::Ident, std::string> paramSubs;
            if (*it == cod::arg) //type arguments are specified, sub them in
            {
                ++it;
                int i = 0;
                while (*it != cod::endOf(cod::arg))
                    paramSubs[td->params[i]] = readType(it);
            }

            //copy in type.
            //alias type parameters which are not specified or aliased
            
            std::string::iterator it2 = td->mapped.expanded.begin();
            while (it2 != td->mapped.expanded.end())
            {
                if (*it2 == cod::param)
                {
                    ++it2;
                    ast::Ident paramName = readNum(it2);
                    if (paramSubs.count(paramName)) //type is specified
                        expanded += paramSubs[paramName]; //replace it
                    else
                    {
                        if (paramName >> 16 == 0) //type is not aliased
                            paramName |= (td->name+1) << 16; //alias it
                        expanded += cod::param + utl::to_str(paramName) + cod::endOf(cod::param); //put it in
                    }
                    ++it2; //skip over param end
                }
                else
                {
                    expanded += *it2;
                    ++it2;
                }
            }
        }
        else if (*it == cod::tupname) //strip out tuple element names
        {
            ++it;
            readNum(it);
        }
        else
        {
            expanded += *it;
            ++it;
        }
    }
}

void Type::parseSingle(lex::Lexer *l)
{
    tok::Token t = l->Peek();
    switch (t.Type())
    {
    case listBegin:
        parseList(l);
        break;

    case tupleBegin:
        parseTuple(l);
        break;

    case tok::k_int:
    case tok::k_float:
        parsePrim(l);
        break;

    case tok::question:
        parseParam(l);
        break;

    case tok::at:
        parseRef(l);
        break;

    case tok::identifier:
        parseNamed(l);
        break;

    default:
        err::Error(t.loc) << "unexpected " << t.Name() << ", expecting type"
            << err::caret << err::endl;
        l->Advance();
    }
}

namespace typ
{
    class TypeListParser
    {
        Type *ty;
    public:
        int num;
        TypeListParser(Type *t) : ty(t), num(0) {}
        void operator()(lex::Lexer *l)
        {
            ++num;
            ty->parseSingle(l);
            if (l->Peek() == tok::identifier)
            {
                ty->code += cod::tupname;
                ty->parseIdent(l);
            }
        }
    };
}

void Type::parseTypeList(lex::Lexer *l)
{
    TypeListParser tlp(this);
    par::parseListOf(l, couldBeType, tlp, tupleEnd, "types");
}

void Type::parseList(lex::Lexer *l)
{
    code += cod::list;
    tok::Location errLoc = l->Next().loc;
    parseSingle(l);
    if (!l->Expect(listEnd))
        err::Error(errLoc) << "unterminated list type" << err::caret << err::endl;
    code += cod::endOf(cod::list);
}

void Type::parseTuple(lex::Lexer *l)
{
    code += cod::tuple;
    l->Advance();
    parseTypeList(l);
    code += cod::endOf(cod::tuple);
}

void Type::parsePrim(lex::Lexer *l)
{
    if (l->Next() == tok::k_int)
        (code += cod::integer) += cod::endOf(cod::integer);
    else 
        (code += cod::floating) += cod::endOf(cod::floating);
}

void Type::parseRef(lex::Lexer *l)
{
    code += cod::ref;
    l->Advance();
    parseSingle(l);
    code += cod::endOf(cod::ref);
}

void Type::parseNamed(lex::Lexer *l)
{
    ast::TypeDef * td = l->getCompUnit()->getTypeDef(l->Peek().value.int_v);

    if (!td)
    {
        err::Error(l->Peek().loc) << "undefined type '"
            << l->getCompUnit()->getIdent(l->Peek().value.int_v) << '\''
            << err::underline << err::endl;
        code += cod::integer + cod::endOf(cod::integer); //recover
        l->Advance(); //don't parse it twice
    }
    else
    {
        code += cod::named;
        parseIdent(l);
    }
    
    tok::Location argsLoc = l->Peek().loc;

    size_t nargs = checkArgs(l);

    argsLoc = argsLoc + l->Last().loc;

    if (td && nargs && nargs != td->params.size())
        err::Error(argsLoc) << "incorrect number of type arguments, expected 0 or " <<
            td->params.size() << ", got " << nargs << err::underline << err::endl;

    code += cod::endOf(cod::named);
}

void Type::parseParam(lex::Lexer *l)
{
    l->Advance();
    if (l->Peek() == tok::identifier)
    {
        code += cod::param;
        parseIdent(l);
        code += cod::endOf(cod::param);
    }
    else
        code += cod::any + cod::endOf(cod::any);
}

size_t Type::checkArgs(lex::Lexer *l)
{
    if (l->Expect(tok::bang))
    {
        code += cod::arg;
        if (l->Expect(tok::lparen))
        {
            TypeListParser tlp(this);
            par::parseListOf(l, couldBeType, tlp, tok::rparen, "types");
            code += cod::endOf(cod::arg);

            return tlp.num;
        }
        else
        {
            parseSingle(l);
            code += cod::endOf(cod::arg);
            
            return 1;
        }
    }
    return 0;
}

void Type::parseIdent(lex::Lexer *l)
{
    code += utl::to_str(l->Next().value.int_v);
}

bool Type::isFunc()
{
    return code.find(cod::function) != std::string::npos;
}

bool Type::isTempl()
{
    return code.find(cod::any) != std::string::npos || code.find(cod::param) != std::string::npos;
}

utl::weak_string Type::ex_w_str()
{
    return utl::weak_string(&*expanded.begin(), &*expanded.end());
}

utl::weak_string Type::w_str()
{
    return utl::weak_string(&*code.begin(), &*code.end());
}
