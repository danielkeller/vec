#include "Scope.h"

using namespace ast;

void NormalScope::addTypeDef(Ident name, TypeDef &td)
{
    typeDefs[name] = td;
}

TypeDef * NormalScope::getTypeDef(Ident name)
{
    auto it = typeDefs.find(name);
    if (it == typeDefs.end())
    {
        if (parent)
            return parent->getTypeDef(name);
        else
            return nullptr;
    }
    else
        return &it->second;
}

void NormalScope::addVarDef(Ident name, DeclExpr* decl)
{
    varDefs[name] = decl;
}

DeclExpr* NormalScope::getVarDef(Ident name)
{
    auto it = varDefs.find(name);
    if (it == varDefs.end())
    {
        if (parent)
            return parent->getVarDef(name);
        else
            return nullptr;
    }
    else
        return it->second;
}

bool NormalScope::canSee(Scope* other)
{
    if (this == other)
        return true;
    else if (!parent)
        return false;
    else
        return getParent()->canSee(other);
}

DeclExpr* ImportScope::getVarDef(Ident name)
{
    //don't cycle
    if (hasVisited)
        return nullptr;
    hasVisited = true;

    for (auto scope : imports)
    {
        DeclExpr* decl = scope->getVarDef(name);
        if (decl)
        {
            hasVisited = false;
            return decl;
        }
    }

    hasVisited = false;
    return nullptr;
}

TypeDef * ImportScope::getTypeDef(Ident name)
{
    //don't cycle
    if (hasVisited)
        return nullptr;
    hasVisited = true;

    for (auto scope : imports)
    {
        TypeDef* decl = scope->getTypeDef(name);
        if (decl)
        {
            hasVisited = false;
            return decl;
        }
    }

    hasVisited = false;
    return nullptr;
}

bool ImportScope::canSee(Scope* other)
{
    if (this == other)
        return true;
    else if (!imports.size())
        return false;

    //don't cycle
    if (hasVisited)
        return false;
    hasVisited = true;
    
    for (auto scope : imports)
    {
        if (scope->canSee(other))
        {
            hasVisited = false;
            return true;
        }
    }

    hasVisited = false;
    return false;
}