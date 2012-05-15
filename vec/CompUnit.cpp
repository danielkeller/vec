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

void CompUnit::addTypeDef(TypeDef &td)
{
    typeDefs[td.name] = td;
}

TypeDef * CompUnit::getTypeDef(Ident name)
{
    std::map<Ident, TypeDef>::iterator it = typeDefs.find(name);
    return it == typeDefs.end() ? 0 : &it->second;
}
