#ifndef SCOPE_H
#define SCOPE_H

#include "Type.h"
#include <vector>
#include <map>

namespace ast
{
    typedef int Ident;
    struct DeclExpr;

    struct TypeDef
    {
        std::vector<Ident> params;
        typ::Type mapped;
    };

    class Scope
    {
    public: //maybe find a better way?

        std::map<Ident, TypeDef> typeDefs;
        std::map<Ident, DeclExpr*> varDefs;
        Scope *parent;

        Scope() : parent(0) {};
        Scope(Scope *p) : parent(p) {};

        //insert var def into current scope
        void addVarDef(Ident name, DeclExpr* decl);
        //recursively find def in all scope parents
        DeclExpr* getVarDef(ast::Ident name);

        //likewise
        void addTypeDef(Ident name, TypeDef & td);
        TypeDef * getTypeDef(ast::Ident name);

        Scope* getParent() {return parent;};
    };
}

#endif
