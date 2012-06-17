#ifndef SEMA_H
#define SEMA_H

#include "CompUnit.h"
#include "AstNode.h"

#include <functional>

namespace sa
{
    class Sema
    {
        ast::CompUnit* cu;

        template<class T>
        void AstWalk(std::function<void(T*)> action);

		void validateTree();

    public:
        Sema(ast::CompUnit* c) : cu(c) {};

        //add/rearrange nodes in AST
        void Phase1();

        //convert AST to basic block graph
        void Phase2();

        //infer types
        void Phase3();

        //any changes that need types
        void Phase4();
    };

    template<class T>
    void Sema::AstWalk(std::function<void(T*)> action)
    {
        AstWalker<T> aw(cu->treeHead, action);
    }
}

#endif
