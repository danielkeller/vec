#include "CompUnit.h"

#include <algorithm>
#include <utility>

using namespace ast;

Str CompUnit::addString(std::string &str)
{
    TblType::iterator it = std::find(stringTbl.begin(), stringTbl.end(), str);
    Str ret = it - stringTbl.begin();
    if (it == stringTbl.end())
        stringTbl.push_back(std::move(str));
    return ret;
}

Ident CompUnit::addIdent(std::string &str)
{
    TblType::iterator it = std::find(identTbl.begin(), identTbl.end(), str);
    Ident ret = it - identTbl.begin();
    if (it == identTbl.end())
        identTbl.push_back(std::move(str));
    return ret;
}

CompUnit::CompUnit()
    : global(new NullExpr())
{
}
