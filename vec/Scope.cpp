#include "Scope.h"

using namespace ast;

void Scope::addTypeDef(Ident name, TypeDef &td)
{
    typeDefs[name] = td;
}

TypeDef * Scope::getTypeDef(Ident name)
{
    std::map<Ident, TypeDef>::iterator it = typeDefs.find(name);
    if (it == typeDefs.end())
    {
        if (parent)
            return parent->getTypeDef(name);
        else
            return 0;
    }
    else
        return &it->second;
}

void Scope::addVarDef(Ident name, VarDef &tr)
{
    varDefs[name] = tr;
}

VarDef * Scope::getVarDef(Ident name)
{
    std::map<Ident, VarDef>::iterator it = varDefs.find(name);
    if (it == varDefs.end())
    {
        if (parent)
            return parent->getVarDef(name);
        else
            return 0;
    }
    else
        return &it->second;
}
