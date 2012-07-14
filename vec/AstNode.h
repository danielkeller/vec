#ifndef ASTNODE_H
#define ASTNODE_H

#include "Token.h"

#include <functional>
#include <ostream>
#include <cassert>
#include <list>
#include <vector>

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
        AstNodeB(tok::Location const &l) : loc(l), parent(0) {};
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

    //node with any number of children
    template<class T>
    struct AstNodeN : public virtual AstNodeB
    {
    private:
        typedef std::vector<T*> conts_t;
        conts_t chld;
        
    public:
        AstNodeN() {}
        AstNodeN(T* first)
        {
            appendChild(first);
        }

        ~AstNodeN()
        {
            for (auto n : chld)
                delete n;
        };

        void eachChild(sa::AstWalker0 *w)
        {
            for (auto n : chld)
                w->call(n);
        }

        void replaceChild(AstNodeB* old, AstNodeB* nw)
        {
            for (T*& n : chld) //specify ref type to be able to modify it
                if (n == old)
                {
                    n = dynamic_cast<T*>(nw);
                    return;
                }
            assert(false && "didn't find child");
        }

        T*& getChild(int n)
        {
            return chld[n];
        }

        const conts_t& Children() const
        {
            return chld;
        }

        void setChild(int n, T* c)
        {
            c->parent = this;
            chld[n] = c;
        }

        void appendChild(T* c)
        {
            c->parent = this;
            chld.push_back(c);
        }

        void popChild()
        {
            chld.pop_back();
        }

        void emitDot(std::ostream &os)
        {
            int p = 0;
            for (auto n : chld)
            {
                os << 'n' << static_cast<AstNodeB*>(this) << ":p" << p <<  " -> n" << static_cast<AstNodeB*>(n) << ";\n";
                ++p;
                n->emitDot(os);
            }
            os << 'n' << static_cast<AstNodeB*>(this) << " [label=\"" << myLbl();
            for (unsigned int p = 0; p < chld.size(); ++p)
                os << "|<p" << p << ">     ";
            os << "\",shape=record,style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }

        void consume(AstNodeN<T>* bb)
        {
            chld.insert(chld.end(), bb->chld.begin(), bb->chld.end());
            for (auto c : chld)
                c->parent = this;
            bb->chld.clear();
            delete bb;
        }

        //split this node in half at the position of the iterator
        //left < posn, middle == posn, right > posn.
        template<class NewNode>
        std::tuple<NewNode*, T*, NewNode*>
        split(typename conts_t::const_iterator posn)
        {
            NewNode* right = new NewNode();
            right->chld.insert(right->chld.begin(), posn + 1, chld.cend());
            chld.erase(chld.begin() + (posn - chld.cbegin()) + 1, chld.end());
            for (auto c : right->chld)
                c->parent = right;
            T* middle = chld.back();
            chld.pop_back();
            NewNode* left = new NewNode();
            left->consume(this);
            return std::make_tuple(left, middle, right);
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
