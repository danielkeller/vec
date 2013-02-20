#include "Scope.h"
#include "Expr.h"

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

void NormalScope::addVarDef(DeclExpr* decl)
{
    varDefs.push_back(decl);
}

void NormalScope::removeVarDef(DeclExpr* decl)
{
    //erase remove the element that is now one past the end
    varDefs.erase(std::remove(varDefs.begin(), varDefs.end(), decl));
}

DeclExpr* NormalScope::getVarDef(Ident name)
{
    for (auto def : varDefs)
        if (def->name == name)
            return def;

    if (parent)
        return parent->getVarDef(name);
    else
        return nullptr;
}

std::vector<DeclExpr*> NormalScope::getVarDefs(Ident name)
{
    std::vector<DeclExpr*> ret;
    if (parent)
        ret = parent->getVarDefs(name);

    for (auto def : varDefs)
        if (def->name == name)
            ret.push_back(def);

    return ret;
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

std::vector<DeclExpr*> ImportScope::getVarDefs(Ident name)
{
    //don't cycle
    if (hasVisited)
        return std::vector<DeclExpr*>();
    hasVisited = true;

    std::vector<DeclExpr*> ret;

    for (auto scope : imports)
    {
        std::vector<DeclExpr*> defs = scope->getVarDefs(name);
        ret.insert(ret.end(), defs.begin(), defs.end());
    }

    hasVisited = false;
    return ret;
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
