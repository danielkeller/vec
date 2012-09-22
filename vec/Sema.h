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
        
    public:
        Sema(ast::Module* c) : mod(c) {};

        //add/rearrange nodes in AST
        void Phase1();

        void Import();
    };
}

#endif
