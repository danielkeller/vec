#include "CompUnit.h"

#include <algorithm>

using namespace ast;

Str CompUnit::addString(std::string &str)
{
    TblType::iterator it = std::find(stringTbl.begin(), stringTbl.end(), str);
    Str ret = it - stringTbl.begin();
    if (it == stringTbl.end())
        stringTbl.push_back(str);
    return ret;
}

Ident CompUnit::addIdent(std::string &str)
{
    TblType::iterator it = std::find(identTbl.begin(), identTbl.end(), str);
    Ident ret = it - identTbl.begin();
    if (it == identTbl.end())
        identTbl.push_back(str);
    return ret;
}

CompUnit::CompUnit()
{
    scopes.push_back(Scope()); //create global scope
}
