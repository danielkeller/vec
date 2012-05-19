#ifndef ASTNODE_H
#define ASTNODE_H

#include <tuple>
#include <type_traits>
#include <functional>

namespace sa
{
    struct AstWalkerProxy;
}

namespace ast
{
    template<std::size_t> struct int2type{};

    //generic AstNode with no children for generic node classes ie expr
    class AstNode0
    {
    public:
        AstNode0 *parent;
        virtual ~AstNode0() {}; 
        virtual void eachChild(sa::AstWalkerProxy &p) {}
    };

    //ast node from which all others are derived
    template<class... Children>
    class AstNode : public virtual AstNode0
    {
        typedef std::tuple<Children* ...> conts_t;
        static const int conts_s = std::tuple_size<conts_t>::value;

        conts_t chld;

        //recurse on number of args
        template<class T, class... Rest, int pos = 0>
        void init(T first, Rest... r)
        {
            first->parent = this;
            std::get<pos>(chld) = first;
            init<pos+1>(r...);
        };
        //terminate recursion
        template<int pos = 0> //need default param for AstNode<>
        void init() {};

        //recurse on int2type type
        template<size_t i = 0>
        void destroy(int2type<i> = int2type<i>())
        {
            delete std::get<i>(chld);
            destroy(int2type<i+1>());
        };
        //terminate template recursion
        void destroy(int2type<conts_s> = int2type<conts_s>()) //need default param for AstNode<>
        {
        };

        template<size_t i = 0>
        void callOnEach(sa::AstWalkerProxy &proxy, int2type<i> = int2type<i>())
        {
            proxy(std::get<i>(chld));
            callOnEach(proxy, int2type<i+1>());
        };
        //terminate recursion
        //need default param for AstNode<>
        void callOnEach(sa::AstWalkerProxy &proxy, int2type<conts_s> = int2type<conts_s>())
        {
        };

    public:
        AstNode(Children* ... c)
        {
            init(c...);
        };

        ~AstNode()
        {
            destroy();
        };

        virtual void eachChild(sa::AstWalkerProxy &p)
        {
            callOnEach(p);
        }

        template<size_t n>
        class std::tuple_element<n, conts_t>::type
        getChild()
        {
            return std::get<n>(chld);
        }

        template<size_t n>
        void setChild(class std::tuple_element<n, conts_t>::type newchld)
        {
            std::get<n>(chld)->parent = 0;
            std::get<n>(chld) = newchld;
            newchld->parent = this;
        }
    };
}

namespace sa
{
    class AstWalker0
    {
        friend struct AstWalkerProxy;
        virtual void actOn(ast::AstNode0* node) = 0;
    };

    //allows us to call a template functor from a virtual function
    struct AstWalkerProxy
    {
        AstWalker0* owner;
        AstWalkerProxy(AstWalker0 *o) : owner(o) {}
        void operator()(ast::AstNode0* node)
        {
            owner->actOn(node);
        }
    };

    template<class Base>
    class AstWalker : public AstWalker0
    {
        ast::AstNode0 *root;
        std::function<void(Base*)> action;
        AstWalkerProxy proxy;
        void actOn(ast::AstNode0* node)
        {
            node->eachChild(proxy);
            Base * bNode = dynamic_cast<Base*>(node);
            if (bNode)
                action(bNode);
        };

    public:
        template <class A>
        AstWalker(A &&act, ast::AstNode0 *r) : root(r), action(act), proxy(this)
        {
            actOn(root);
        }
    };
}


#endif
