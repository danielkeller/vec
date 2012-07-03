#include "Scope.h"

using namespace ast;

void Scope::addTypeDef(Ident name, TypeDef &td)
{
    typeDefs[name] = td;
}

TypeDef * Scope::getTypeDef(Ident name)
{
    auto it = typeDefs.find(name);
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

void Scope::addVarDef(Ident name, DeclExpr* decl)
{
    varDefs[name] = decl;
}

DeclExpr* Scope::getVarDef(Ident name)
{
    auto it = varDefs.find(name);
    if (it == varDefs.end())
    {
        if (parent)
            return parent->getVarDef(name);
        else
            return 0;
    }
    else
        return it->second;
}

bool Scope::canSee(Scope* other)
{
    Scope * search;
    for (search = this; search && search != other; search = search->parent)
        ;
    return search == other;
}