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
