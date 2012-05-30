#ifndef ASTNODE_H
#define ASTNODE_H

#include <tuple>
#include <type_traits>
#include <functional>
#include <ostream>

namespace ast
{
    class AstNode0;
}

namespace sa
{
    //allows us to call a template functor from a virtual function
    class AstWalker0
    {
    public:
        virtual void call(ast::AstNode0* node) = 0;
    };
}

namespace ast
{
    template<std::size_t> struct int2type{};

    //virtual AstNode base class for generic node classes ie expr, stmt
    class AstNode0
    {
    public:
        AstNode0 *parent;
        virtual ~AstNode0() {}; 
        virtual void eachChild(sa::AstWalker0*) = 0;
        virtual void emitDot(std::ostream &os) = 0;
        //replace child of unknown position
        virtual void replaceChild(AstNode0* old, AstNode0* n) = 0;
        virtual std::string myLbl()
        {
            return typeid(*this).name();
        };
        virtual const char* myColor()
        {
            return "1";
        };
    };

    //ast node from which all others are derived
    template<class... Children>
    class AstNode : public virtual AstNode0
    {
        typedef std::tuple<Children* ...> conts_t;
        static const int conts_s = std::tuple_size<conts_t>::value;

        conts_t chld;

        //recurse on int2type type
        template<size_t i>
        inline void init(int2type<i>)
        {
            std::get<i>(chld)->parent = this;
            init(int2type<i+1>());
        };
        //terminate template recursion
        inline void init(int2type<conts_s>)
        {
        };

        template<size_t i>
        inline void destroy(int2type<i>)
        {
            delete std::get<i>(chld);
            destroy(int2type<i+1>());
        };
        //terminate template recursion
        inline void destroy(int2type<conts_s>) //need default param for AstNode<>
        {
        };

        template<size_t i>
        inline void callOnEach(sa::AstWalker0 *walker, int2type<i>)
        {
            walker->call(std::get<i>(chld));
            callOnEach(walker, int2type<i+1>());
        };
        //terminate recursion
        void callOnEach(sa::AstWalker0*, int2type<conts_s>)
        {
        };

        template<size_t i>
        inline void chldDot(std::ostream &os, int2type<i>)
        {
            //static cast is needed to get pointers to the same thing to be the same
            os << 'n' << static_cast<AstNode0*>(this) << " -> n" << static_cast<AstNode0*>(std::get<i>(chld)) << ";\n";
            std::get<i>(chld)->emitDot(os);
            chldDot(os, int2type<i+1>());
        };
        //terminate template recursion
        inline void chldDot(std::ostream &os, int2type<conts_s>)
        {
        };

        template<size_t i>
        inline void replaceWith(AstNode0* old, AstNode0* n, int2type<i>)
        {
            //cast the old type to the child's type to compare correctly
            typename std::tuple_element<i, conts_t>::type oldc
                = dynamic_cast<typename std::tuple_element<i, conts_t>::type>(old);

            if (oldc == std::get<i>(chld))
                setChild<i>(dynamic_cast<decltype(oldc)>(n));
            else
                replaceWith(old, n, int2type<i+1>());
        };
        //terminate recursion
        void replaceWith(AstNode0*, AstNode0*, int2type<conts_s>)
        {
        };

    public:
        AstNode(Children* ... c)
            : chld(c...)
        {
            init(int2type<0>());
        };

        ~AstNode()
        {
            destroy(int2type<0>());
        };

        void eachChild(sa::AstWalker0 *w)
        {
            callOnEach(w, int2type<0>());
        }

        void replaceChild(AstNode0* old, AstNode0* n)
        {
            replaceWith(old, n, int2type<0>());
        }

        template<size_t n>
        typename std::tuple_element<n, conts_t>::type &
        getChild()
        {
            return std::get<n>(chld);
        }

        template<size_t n>
        void setChild(typename std::tuple_element<n, conts_t>::type newchld)
        {
            std::get<n>(chld)->parent = 0;
            std::get<n>(chld) = newchld;
            newchld->parent = this;
        }

        void emitDot(std::ostream &os)
        {
            chldDot(os, int2type<0>());
            os << 'n' << static_cast<AstNode0*>(this) << " [label=\"" << myLbl()
                << "\",style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }

    };
}

namespace sa
{
    template<class Base>
    class AstWalker : public AstWalker0
    {
        std::function<void(Base*)> action;

    public:
        void call(ast::AstNode0* node)
        {
            node->eachChild(this);
            Base * bNode = dynamic_cast<Base*>(node);
            if (bNode)
                action(bNode);
        };
        template <class A>
        AstWalker(A act, ast::AstNode0 *r) : action(act)
        {
            call(r);
        }
    };
}


#endif
