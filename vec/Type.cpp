#include "Type.h"
#include "Util.h"
#include "Error.h"
#include "CompUnit.h"
#include "Parser.h"

#include <cstdlib>

using namespace par;
using namespace typ;

namespace
{
    int readNum(std::string::iterator &it)
    {
        char *end;
        int ret = strtol(&*it, &end, 10);
        it += (end - &*it);
        return ret;
    }

    std::string::iterator endOfType(std::string::iterator it, bool skipName = true)
    {
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

        if (skipName && *it == cod::tupname) //special case
            readNum(++it);

        return it;
    }

    std::string readType(std::string::iterator &it)
    {
        std::string::iterator start = it;
        it = endOfType(it);
        return std::string(start, it);
    }

    std::pair<int, int> readNumAndOwner(std::string::iterator &it)
    {
        char *end;

        int alias = strtol(&*it, &end, 10);
        it += (end - &*it);

        return std::make_pair(alias & 0xFFFF, (alias >> 16) - 1);
    }

    inline bool isType(char c)
    {
        return c >= 'A' && c <= 'Z';
    }
}

Type::Type()
    : code(""), expanded("")
{}

Type::Type(TypeIter &ti)
{
    code.assign(ti.pos, endOfType(ti.pos, false));
}

void Type::expand(ast::Scope *s)
{
    //fill in expnaded type
    expanded.clear();
    std::string::iterator it = code.begin();

    while (it != code.end())
    {
        if (*it == cod::named) //fill in named types
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
                        expanded += paramSubs[paramName]; //replace it
                    else
                    {
                        if (paramName >> 16 == 0) //param is not aliased
                            paramName |= (id + 1) << 16; //alias it
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

        else if (*it == cod::dim) //strip out list dimensions
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

TypeIter Type::begin(bool arg)
{
    //beginning of argument type corresponds to end of return type + 1
    //will return invalid iterator if called on non-function type
    return arg ? end(false) + 1: code.begin();
}

TypeIter Type::end(bool arg)
{
    return arg ? code.end() : std::find(code.begin(), code.end(), cod::function); //returns end if not function
}

TypeIter Type::exbegin(bool arg)
{
    return arg ? end(false) + 1: expanded.begin();
}

TypeIter Type::exend(bool arg)
{
    return arg ? expanded.end() : std::find(expanded.begin(), expanded.end(), cod::function); //returns end if not function
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

TypeIter TypeIter::operator+(size_t offset)
{
    TypeIter ret(*this);
    return ret += offset;
}

TypeIter& TypeIter::operator+=(size_t offset)
{
    pos += offset;
    return *this;
}

TypeIter& TypeIter::operator++()
{
    //go to next type
    while(!isType(*pos) && *pos != cod::function && *pos != '\0')
        ++pos;

    return *this;
}

void TypeIter::descend()
{
    ++pos;
}

bool TypeIter::atBottom()
{
    return !isType(*(pos + 1));
}

void TypeIter::advance()
{
    pos = endOfType(pos);
}

bool TypeIter::atEnd()
{
    return !isType(*(endOfType(pos)));
}

void TypeIter::ascend()
{
    operator++();
}

bool TypeIter::atTop()
{
    return *this != ++TypeIter(*this);
}

ast::Ident TypeIter::getName()
{
    std::string::iterator it = pos;
    switch (*pos)
    {
    case cod::arg:
    case cod::named:
    case cod::param:
        return readNum(it);
    default:
        return -1;
    }
}

ast::Ident TypeIter::getTupName()
{
    std::string::iterator it = endOfType(pos, false);
    if (*++it == cod::tupname)
        return readNum(it);
    else
        return -1;
}

utl::weak_string TypeIter::w_str()
{
    return utl::weak_string(&*pos, &*endOfType(pos, false));
}
