#ifndef ASTNODE_H
#define ASTNODE_H

#include <tuple>
#include <type_traits>
#include <functional>

namespace sa
{
    class AstWalker0;
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
        virtual void eachChild(sa::AstWalker0*) {}
    };

    //ast node from which all others are derived
    template<class... Children>
    class AstNode : public virtual AstNode0
    {
        typedef std::tuple<Children* ...> conts_t;
        static const int conts_s = std::tuple_size<conts_t>::value;

        conts_t chld;

        //recurse on int2type type
        template<size_t i = 0>
        inline void init(int2type<i> = int2type<i>())
        {
            std::get<i>(chld)->parent = this;
            destroy(int2type<i+1>());
        };
        //terminate template recursion
        inline void init(int2type<conts_s> = int2type<conts_s>()) //need default param for AstNode<>
        {
        };

        template<size_t i = 0>
        inline void destroy(int2type<i> = int2type<i>())
        {
            delete std::get<i>(chld);
            destroy(int2type<i+1>());
        };
        //terminate template recursion
        inline void destroy(int2type<conts_s> = int2type<conts_s>()) //need default param for AstNode<>
        {
        };

        template<size_t i = 0>
        inline void callOnEach(sa::AstWalker0 *walker, int2type<i> = int2type<i>())
        {
            walker(std::get<i>(chld));
            callOnEach(walker, int2type<i+1>());
        };
        //terminate recursion
        //need default param for AstNode<>
        void callOnEach(sa::AstWalker0*, int2type<conts_s> = int2type<conts_s>())
        {
        };

    public:
        AstNode(Children* ... c)
            : chld(c...)
        {
            init();
        };

        ~AstNode()
        {
            destroy();
        };

        virtual void eachChild(sa::AstWalker0 *w)
        {
            callOnEach(w);
        }

        template<size_t n>
        typename std::tuple_element<n, conts_t>::type
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
    //allows us to call a template functor from a virtual function
    class AstWalker0
    {
        friend class AstNode0;
        virtual void operator()(ast::AstNode0* node) = 0;
    };

    template<class Base>
    class AstWalker : public AstWalker0
    {
        std::function<void(Base*)> action;
        void operator()(ast::AstNode0* node)
        {
            node->eachChild(this);
            Base * bNode = dynamic_cast<Base*>(node);
            if (bNode)
                action(bNode);
        };

    public:
        template <class A>
        AstWalker(A &&act, ast::AstNode0 *r) : action(act)
        {
            operator()(r);
        }
    };
}


#endif
