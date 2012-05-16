#ifndef SCOPE_H
#define SCOPE_H

#include "Type.h"
#include <vector>
#include <map>

namespace ast
{
    typedef int Ident;

    struct TypeDef
    {
        Ident name;
        std::vector<Ident> params;
        typ::Type mapped;
    };

    class Scope
    {
        std::map<Ident, TypeDef> typeDefs;
        Scope *parent;

    public:
        Scope() : parent(0) {};
        Scope(Scope *p) : parent(p) {};
        void addTypeDef(TypeDef & td);
        TypeDef * getTypeDef(ast::Ident name);
    };
}

#endif
