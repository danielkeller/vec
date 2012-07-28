#ifndef ASTWALKER_H
#define ASTWALKER_H

#include "AstNode.h"

namespace sa
{
    //walks up the tree from the bottom
    template<class Base>
    class AstWalker : public AstWalker0
    {
        std::function<void(Base*)> action;

    public:
        void call(ast::Node0* node)
        {
            node->eachChild(this);
            Base * bNode = exact_cast<Base*>(node);
            if (bNode)
                action(bNode);
        };
        template <class A>
        AstWalker(ast::Node0 *r, A act) : action(act)
        {
            call(r);
        }
    };

    template<class Base>
    class DynamicAstWalker : public AstWalker0
    {
        std::function<void(Base*)> action;

    public:
        void call(ast::Node0* node)
        {
            node->eachChild(this);
            Base * bNode = dynamic_cast<Base*>(node);
            if (bNode)
                action(bNode);
        };
        template <class A>
        DynamicAstWalker(ast::Node0 *r, A act) : action(act)
        {
            call(r);
        }
    };

    class AnyAstWalker : public AstWalker0
    {
        std::function<void(ast::Node0*)> action;

    public:
        void call(ast::Node0* node)
        {
            node->eachChild(this);
            action(node);
        };
        template <class A>
        AnyAstWalker(ast::Node0 *r, A act) : action(act)
        {
            call(r);
        }
    };

    //walks down the tree from the root
    template<class Base>
    class ReverseAstWalker : public AstWalker0
    {
        std::function<void(Base*)> action;

    public:
        void call(ast::Node0* node)
        {
            Base * bNode = exact_cast<Base*>(node);
            if (bNode)
                action(bNode);
            node->eachChild(this);
        };
        template <class A>
        ReverseAstWalker(ast::Node0 *r, A act) : action(act)
        {
            call(r);
        }
    };

    //walks up the tree, but stores the results so more destructive
    //modifications can take place without crashing
    //you still may not delete any nodes in the cache
    template<class Base>
    class CachedAstWalker : public AstWalker0
    {
        std::function<void(Base*)> action;
        std::list<Base*> cache;

    public:
        void call(ast::Node0* node)
        {
            node->eachChild(this);
            Base * bNode = exact_cast<Base*>(node);
            if (bNode)
                cache.push_back(bNode);
        };
        template <class A>
        CachedAstWalker(ast::Node0 *r, A act) : action(act)
        {
            call(r);
            for (auto it : cache)
                action(it);
        }
    };
}

#endif