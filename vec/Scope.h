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
        std::vector<Ident> params;
        typ::Type mapped;
    };

    struct VarDef
    {
        typ::Type type;
        //Expression init;
    };

    class Scope
    {
        std::map<Ident, TypeDef> typeDefs;
        std::map<Ident, VarDef> varDefs;
        Scope *parent;

    public:
        Scope() : parent(0) {};
        Scope(Scope *p) : parent(p) {};

        void addTypeDef(Ident name, TypeDef & td);
        TypeDef * getTypeDef(ast::Ident name);

        void addVarDef(Ident name, VarDef & vd);
        VarDef * getVarDef(ast::Ident name);
    };
}

#endif
