#ifndef SCOPE_H
#define SCOPE_H

#include "Type.h"
#include <vector>
#include <map>

namespace ast
{
    struct DeclExpr;

    struct TypeDef
    {
        std::vector<Ident> params;
        typ::Type mapped;
    };

    class Scope
    {
    public:
        virtual DeclExpr* getVarDef(Ident name) = 0;
        virtual TypeDef * getTypeDef(Ident name) = 0;

        virtual bool canSee(Scope* other) = 0;

        virtual ~Scope() {};
    };

    class NormalScope : public Scope
    {
        Scope *parent;

    public: //maybe find a better way?

        std::map<Ident, TypeDef> typeDefs;
        std::map<Ident, DeclExpr*> varDefs;

        //insert var def into current scope
        void addVarDef(Ident name, DeclExpr* decl);
        //recursively find def in all scope parents
        DeclExpr* getVarDef(Ident name);

        NormalScope() : parent(0) {};
        NormalScope(Scope *p) : parent(p) {};

        //likewise
        void addTypeDef(Ident name, TypeDef & td);
        TypeDef * getTypeDef(Ident name);

        Scope* getParent() {return parent;};

        bool canSee(Scope* other);
    };

    class ImportScope : public Scope
    {
        std::vector<Scope*> imports;
        bool hasVisited; //to stop cycles

    public:
        ImportScope(Scope *p) : hasVisited(false)
        {
            imports.push_back(p);
        }

        void Import(Scope* other)
        {
            imports.push_back(other);
        }

        DeclExpr* getVarDef(Ident name);
        TypeDef * getTypeDef(Ident name);

        bool canSee(Scope* other);
    };
}

#endif
