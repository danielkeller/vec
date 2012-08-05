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

        void validateTree();

        void processFunc (ast::Node0* n);
        
    public:
        Sema(ast::Module* c) : mod(c) {};

        //add/rearrange nodes in AST
        void Phase1();

        //convert AST to basic block graph
        void Phase2();

        //phase 2.5. bring in external symbols.
        void Import();

        //infer types
        void Phase3();
        template<class T>
        void resolveOverload(ast::OverloadGroupDeclExpr* oGroup, T* call, typ::Type argType);

        //constant propigation
        void Phase4();
    };

    //finds the last non-block expression in a list of them
    ast::Node0* findEndExpr(ast::Node0* srch);
}

#endif
