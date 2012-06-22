#ifndef ASTNODE_H
#define ASTNODE_H

#include <functional>
#include <ostream>
#include <cassert>
#include <list>

#ifdef _WIN32
#pragma warning (disable: 4250 4127) //inherits via dominance, cond expr is constant
#endif

namespace ast
{
    class AstNodeB;
}

namespace sa
{
    //allows us to call a template functor from a virtual function
    class AstWalker0
    {
    public:
        virtual void call(ast::AstNodeB* node) = 0;
    };
}

namespace ast
{
    //virtual AstNode base class for generic node classes ie expr, stmt

    class AstNodeB
    {
    public:
        tok::Location loc; //might not be set
        AstNodeB *parent;
        AstNodeB(tok::Location &l) : loc(l), parent(0) {};
        AstNodeB() : parent(0) {};
        virtual ~AstNodeB() {};
        virtual void eachChild(sa::AstWalker0*) = 0;

        virtual void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<AstNodeB*>(this) << " [label=\"" << myLbl()
                << "\",style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }

        //replace child of unknown position
        virtual void replaceChild(AstNodeB*, AstNodeB*)
        {
            assert(false && "didn't find child");
        }

        virtual std::string myLbl()
        {
            return typeid(*this).name();
        }

        virtual const char* myColor()
        {
            return "1";
        }
    };

    class AstNode0 : public virtual AstNodeB
    {
    public:
        AstNode0() {};
        ~AstNode0() {};
        void eachChild(sa::AstWalker0*) {};
    };

    //ast node from which all others are derived
    template<class A>
    class AstNode1 : public AstNode0
    {
        A* a;
    public:
        AstNode1(A* a_i)
            : a(a_i)
        {
            a->parent = this;
        }

        ~AstNode1()
        {
            delete a;
        }

        void eachChild(sa::AstWalker0 *w)
        {
            w->call(a);
        }

        void replaceChild(AstNodeB* old, AstNodeB* n)
        {
            //cast the old type to the child's type to compare correctly
            if (dynamic_cast<A*>(old) == a)
                setChildA(dynamic_cast<A*>(n));
            else
                AstNode0::replaceChild(old, n);
        }

        A* getChildA()
        {
            return a;
        }

        void setChildA(A* newchld)
        {
            assert(newchld && "replacing child with incompatible type");
            a = newchld;
            newchld->parent = this;
        }

        //unlink if we're about to delete this
        void nullChildA()
        {
            a = 0;
        }

        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<AstNodeB*>(this) << " -> n" << static_cast<AstNodeB*>(a) << ";\n";
            a->emitDot(os);
            AstNode0::emitDot(os);
        }
    };

    template<class A, class B>
    class AstNode2 : public AstNode1<A>
    {
        B* b;

    public:
        AstNode2(A* a_i, B* b_i)
            : AstNode1<A>(a_i), b(b_i)
        {
            b->parent = this;
        }

        ~AstNode2()
        {
            delete b;
        }

        void eachChild(sa::AstWalker0 *w)
        {
            AstNode1<A>::eachChild(w);
            w->call(b);
        }

        void replaceChild(AstNodeB* old, AstNodeB* n)
        {
            //cast the old type to the child's type to compare correctly
            if (dynamic_cast<B*>(old) == b)
                setChildB(dynamic_cast<B*>(n));
            else
                AstNode1<A>::replaceChild(old, n);
        }

        B* getChildB()
        {
            return b;
        }

        void setChildB(B* newchld)
        {
            assert(newchld && "replacing child with incompatible type");
            b = newchld;
            newchld->parent = this;
        }

        void nullChildB()
        {
            b = 0;
        }

        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<AstNodeB*>(this) << " -> n" << static_cast<AstNodeB*>(b) << ";\n";
            b->emitDot(os);
            AstNode1<A>::emitDot(os);
        }
    };

    template<class A, class B, class C>
    class AstNode3 : public AstNode2<A, B>
    {
        C* c;

    public:
        AstNode3(A* a_i, B* b_i, C* c_i)
            : AstNode2<A, B>(a_i, b_i), c(c_i)
        {
            c->parent = this;
        }

        ~AstNode3()
        {
            delete c;
        }

        void eachChild(sa::AstWalker0 *w)
        {
            AstNode2<A, B>::eachChild(w);
            w->call(c);
        }

        void replaceChild(AstNodeB* old, AstNodeB* n)
        {
            //cast the old type to the child's type to compare correctly
            if (dynamic_cast<C*>(old) == c)
                setChildC(dynamic_cast<C*>(n));
            else
                AstNode2<A, B>::replaceChild(old, n);
        }

        C* getChildC()
        {
            return c;
        }

        void setChildC(C* newchld)
        {
            assert(newchld && "replacing child with incompatible type");
            c = newchld;
            newchld->parent = this;
        }

        void nullChildC()
        {
            c = 0;
        }

        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<AstNodeB*>(this) << " -> n" << static_cast<AstNodeB*>(c) << ";\n";
            c->emitDot(os);
            AstNode2<A, B>::emitDot(os);
        }
    };
}

namespace sa
{
    //walks up the tree from the bottom
    template<class Base>
    class AstWalker : public AstWalker0
    {
        std::function<void(Base*)> action;

    public:
        void call(ast::AstNodeB* node)
        {
            node->eachChild(this);
            Base * bNode = dynamic_cast<Base*>(node);
            if (bNode)
                action(bNode);
        };
        template <class A>
        AstWalker(ast::AstNodeB *r, A act) : action(act)
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
        void call(ast::AstNodeB* node)
        {
            Base * bNode = dynamic_cast<Base*>(node);
            if (bNode)
                action(bNode);
            node->eachChild(this);
        };
        template <class A>
        ReverseAstWalker(ast::AstNodeB *r, A act) : action(act)
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
        void call(ast::AstNodeB* node)
        {
            node->eachChild(this);
            Base * bNode = dynamic_cast<Base*>(node);
            if (bNode)
                cache.push_back(bNode);
        };
        template <class A>
        CachedAstWalker(ast::AstNodeB *r, A act) : action(act)
        {
            call(r);
            for (auto it : cache)
                action(it);
        }
    };
}


#endif
