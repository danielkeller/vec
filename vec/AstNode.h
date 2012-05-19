#ifndef ASTNODE_H
#define ASTNODE_H

#include <tuple>
#include <type_traits>

namespace ast
{
    template<std::size_t> struct int2type{};

    //generic AstNode with no children for generic node classes ie expr
    //classes should inherit this virtually
    class AstNode0
    {
    public:
        AstNode0 *parent;
        virtual ~AstNode0() {}; 
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

        template<class A, size_t i = 0>
        void callOnEach(A action, int2type<i> = int2type<i>())
        {
            action(std::get<i>(chld));
            callOnEach(action, int2type<i+1>());
        };
        //terminate recursion
        template<class A> //need default param for AstNode<>
        void callOnEach(A action, int2type<conts_s> = int2type<conts_s>())
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

        template<class A>
        void eachChld(A action)
        {
            callOnEach(action);
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


#endif
