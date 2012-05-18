#include "Type.h"
#include "Lexer.h"
#include "Util.h"
#include "Error.h"
#include "CompUnit.h"
#include "ParseUtils.h"

#include <stack>
#include <cstdlib>

using namespace par;
using namespace typ;

namespace cod
{
    static const char list = 'L';
    static const char tuple = 'T';
    static const char tupname = 'v'; //name of variable for struct-style access
    static const char dim = 'd'; //dimension of list
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
    : code(""), expanded("")
{}

Type TypeParser::operator()(lex::Lexer *lex, ast::Scope *sco)
{
    l = lex;
    s = sco;
    t = Type();

    parseSingle();
    if (l->Expect(tok::colon)) //function?
    {
        t.code += cod::function;
        parseSingle();
    }

    //fill in expnaded type
    t.expanded.clear();
    std::string::iterator it = t.code.begin();

    while (it != t.code.end())
    {
        if (*it == cod::named) //fill in named pares
        {
            ++it;
            ast::Ident id = readNum(it);
            ++it; //remove 'n'
            ast::TypeDef *td = s->getTypeDef(id);

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
                    if (paramSubs.count(paramName)) //param is specified
                        t.expanded += paramSubs[paramName]; //replace it
                    else
                    {
                        if (paramName >> 16 == 0) //param is not aliased
                            paramName |= (id + 1) << 16; //alias it
                        t.expanded += cod::param + utl::to_str(paramName) + cod::endOf(cod::param); //put it in
                    }
                    ++it2; //skip over param end
                }
                else
                {
                    t.expanded += *it2;
                    ++it2;
                }
            }
        }

        else if (*it == cod::tupname) //strip out tuple element names
        {
            ++it;
            readNum(it);
        }

        else if (*it == cod::dim) //strip out list dimensions
        {
            ++it;
            readNum(it);
        }
        else
        {
            t.expanded += *it;
            ++it;
        }
    }

    return t;
}

void TypeParser::parseSingle()
{
    tok::Token to = l->Peek();
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
        l->Advance();
    }
}

namespace par
{
    class TypeListParser
    {
        TypeParser *tp;
    public:
        int num;
        TypeListParser(TypeParser *t) : tp(t), num(0) {}
        void operator()(lex::Lexer *l)
        {
            ++num;
            tp->parseSingle();
            if (l->Peek() == tok::identifier)
            {
                tp->t.code += cod::tupname;
                tp->parseIdent();
            }
        }
    };
}

void TypeParser::parseTypeList()
{
    TypeListParser tlp(this);
    par::parseListOf(l, couldBeType, tlp, tupleEnd, "pares");
}

void TypeParser::parseList()
{
    t.code += cod::list;
    tok::Location errLoc = l->Next().loc;
    parseSingle();

    if (!l->Expect(listEnd))
        err::Error(errLoc) << "unterminated list type" << err::caret << err::endl;

    if (l->Expect(tok::bang))
    {
        tok::Token to;
        if (l->Expect(tok::integer, to))
        {
            t.code += cod::dim + utl::to_str(to.value.int_v);
        }
        else
            err::ExpectedAfter(l, "list length", "'!'");
    }
    t.code += cod::endOf(cod::list);
}

void TypeParser::parseTuple()
{
    t.code += cod::tuple;
    l->Advance();
    parseTypeList();
    t.code += cod::endOf(cod::tuple);
}

void TypeParser::parsePrim()
{
    char c;

    if (l->Next() == tok::k_int)
        c = cod::integer;
    else 
        c = cod::floating;

    int width = 32;

    if (l->Expect(tok::bang))
    {
        tok::Token to;
        if (l->Expect(tok::integer, to))
        {
            width = to.value.int_v;

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
            err::ExpectedAfter(l, "numeric width", "'!'");
    }

    t.code += c + utl::to_str(width) + cod::endOf(c);
}

void TypeParser::parseRef()
{
    t.code += cod::ref;
    l->Advance();
    parseSingle();
    t.code += cod::endOf(cod::ref);
}

void TypeParser::parseNamed()
{
    ast::TypeDef * td = s->getTypeDef(l->Peek().value.int_v);

    if (!td)
    {
        err::Error(l->Peek().loc) << "undefined type '"
            << l->getCompUnit()->getIdent(l->Peek().value.int_v) << '\''
            << err::underline << err::endl;
        t.code += cod::integer + cod::endOf(cod::integer); //recover
        l->Advance(); //don't parse it twice
    }
    else
    {
        t.code += cod::named;
        parseIdent();
    }
    
    tok::Location argsLoc = l->Peek().loc;

    size_t nargs = checkArgs();

    argsLoc = argsLoc + l->Last().loc;

    if (td && nargs && nargs != td->params.size())
        err::Error(argsLoc) << "incorrect number of type arguments, expected 0 or " <<
            td->params.size() << ", got " << nargs << err::underline << err::endl;

    t.code += cod::endOf(cod::named);
}

void TypeParser::parseParam()
{
    l->Advance();
    if (l->Peek() == tok::identifier)
    {
        t.code += cod::param;
        parseIdent();
        t.code += cod::endOf(cod::param);
    }
    else
        t.code += cod::any + cod::endOf(cod::any);
}

size_t TypeParser::checkArgs()
{
    if (l->Expect(tok::bang))
    {
        t.code += cod::arg;
        if (l->Expect(tok::lparen))
        {
            TypeListParser tlp(this);
            par::parseListOf(l, couldBeType, tlp, tok::rparen, "pares");
            t.code += cod::endOf(cod::arg);

            return tlp.num;
        }
        else
        {
            parseSingle();
            t.code += cod::endOf(cod::arg);
            
            return 1;
        }
    }
    return 0;
}

void TypeParser::parseIdent()
{
    t.code += utl::to_str(l->Next().value.int_v);
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
