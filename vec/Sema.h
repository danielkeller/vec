#ifndef SEMA_H
#define SEMA_H

#include "Module.h"
#include "SemaNodes.h"

#include <functional>

namespace sa
{
    class Sema
    {
        ast::Module* mod;

        template<class T>
        void AstWalk(std::function<void(T*)> action);
        template<class T>
        void ReverseAstWalk(std::function<void(T*)> action);
        template<class T>
        void CachedAstWalk(std::function<void(T*)> action);

        void validateTree();

        void resolveOverload(ast::OverloadGroupDeclExpr* oGroup, ast::OverloadableExpr* call, typ::Type argType);
        
    public:
        Sema(ast::Module* c) : mod(c) {};

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
        AstWalker<T> aw(mod, action);
    }

    template<class T>
    void Sema::ReverseAstWalk(std::function<void(T*)> action)
    {
        ReverseAstWalker<T> aw(mod, action);
    }

    template<class T>
    void Sema::CachedAstWalk(std::function<void(T*)> action)
    {
        CachedAstWalker<T> aw(mod, action);
    }

    //finds the last non-block expression in a list of them
    ast::Expr* findEndExpr(ast::AstNodeB* srch);
}

#endif
