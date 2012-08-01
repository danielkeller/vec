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

        template<class T>
        void AstWalk(std::function<void(T*)> action);
        template<class T>
        void DynamicAstWalk(std::function<void(T*)> action);
        void AnyAstWalk(std::function<void(ast::Node0*)> action);
        template<class T>
        void ReverseAstWalk(std::function<void(T*)> action);
        template<class T>
        void CachedAstWalk(std::function<void(T*)> action);

        void validateTree();

        template<class T>
        void resolveOverload(ast::OverloadGroupDeclExpr* oGroup, T* call, typ::Type argType);
        void Sema::inferTypes (ast::BasicBlock* bb);
        friend struct Sema3AstWalker;
        
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

        //constant propigation
        void Phase4();
    };

    template<class T>
    void Sema::AstWalk(std::function<void(T*)> action)
    {
        AstWalker<T> aw(mod, action);
    }

    template<class T>
    void Sema::DynamicAstWalk(std::function<void(T*)> action)
    {
        DynamicAstWalker<T> aw(mod, action);
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
    ast::Node0* findEndExpr(ast::Node0* srch);
}

#endif
