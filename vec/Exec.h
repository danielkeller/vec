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

        //this keeps track of variables that may or may not be assigned to at runtime
        //in the course of pre-execution, and sets them to be indeterminate at the approprate
        //time
        std::stack<std::vector<ast::Node0*>> valScopes;

        void processFunc(ast::FuncDeclExpr* n);
        void processMod(ast::Module* mod);

    public:
        Exec(ast::Module* mainMod);

        template<class T>
        void resolveOverload(ast::OverloadGroupDeclExpr* oGroup, T* call, typ::Type argType);

        void pushValScope(); //FIXME: better RT algo
        void popValScope();
        void trackVal(ast::Node0* n);
    };
}

#endif