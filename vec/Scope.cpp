#include "Scope.h"

using namespace ast;

void Scope::addTypeDef(TypeDef &td)
{
    typeDefs[td.name] = td;
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
