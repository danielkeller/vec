#ifndef SEMA_H
#define SEMA_H

#include "Module.h"
#include "SemaNodes.h"
#include "AstWalker.h"

#include <functional>

namespace sa
{
    class Sema
    {
        ast::Module* mod;

        std::map<ast::OverloadableExpr*, std::list<ast::TmpExpr*>> intrins;

        void validateTree();

        void processFunc (ast::FuncDeclExpr* n);
        void processSubtree (ast::Node0* n);
        void processMod();
        
    public:
        Sema(ast::Module* c) : mod(c) {};

        //add/rearrange nodes in AST
        void Phase1();

        void Import();

        //infer types. only called on main module
        void Types();
        template<class T>
        void resolveOverload(ast::OverloadGroupDeclExpr* oGroup, T* call, typ::Type argType);
    };

    //finds the last non-block expression in a list of them
    ast::Node0* findEndExpr(ast::Node0* srch);
}

#endif
