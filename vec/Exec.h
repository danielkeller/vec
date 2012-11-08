#ifndef EXEC_H
#define EXEC_H

#include "SemaNodes.h"
#include <vector>
#include <stack>

namespace sa
{
    //compile-time execution engine
    class Exec
    {
        ast::FuncDeclExpr* curFunc;

        void processFunc(ast::FuncDeclExpr* n);
        void processMod(ast::Module* mod);

    public:
        Exec(ast::Module* mainMod);

        template<class T>
        void resolveOverload(ast::OverloadGroupDeclExpr* oGroup, T* call, typ::Type argType);
    };
}

#endif