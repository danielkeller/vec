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
        void processMod(ast::Module* mod);

    public:
        Exec(ast::Module* mainMod);

        void processFunc(ast::DeclExpr* n);
    };
}

#endif
