#ifndef ASTNODE_H
#define ASTNODE_H

#include "Token.h"
#include "Type.h"

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
    class Node0;
}

namespace sa
{
    //allows us to call a template functor from a virtual function
    class AstWalker0
    {
    public:
        virtual void call(ast::Node0* node) = 0;
    };
}

namespace ast
{
    //virtual Node base class for generic node classes ie expr, stmt

    class Node0
    {
    protected:
        //type is private, and has a virtual "sgetter" so we can override its behavior
        //when a derived Expr has some sort of deterministic type. wasted space isn't
        //a big deal since it's only sizeof(void*) bytes
        typ::Type type;

    public:
        virtual typ::Type& Type() {return type;} //sgetter. sget it?
        virtual bool isLval() {return false;}
        virtual bool isExpr() {return true;}

        tok::Location loc; //might not be set
        Node0 *parent;
        Node0(tok::Location const &l) : loc(l), parent(0) {};

        virtual ~Node0() {};
        virtual void eachChild(sa::AstWalker0*) {};

        virtual void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<Node0*>(this) << " [label=\"" << myLbl()
                << "\",style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }

        //replace child of unknown position
        virtual void replaceChild(Node0*, Node0*)
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

    //ast node from which all others are derived
    class Node1 : public Node0
    {
        Node0* a;
    public:
        Node1(Node0* a_i, tok::Location const &l)
            : Node0(l), a(a_i)
        {
            a->parent = this;
        }

        Node1(Node0* a_i)
            : Node0(a_i->loc), a(a_i)
        {
            a->parent = this;
        }

        ~Node1()
        {
            delete a;
        }

        void eachChild(sa::AstWalker0 *w)
        {
            w->call(a);
        }

        void replaceChild(Node0* old, Node0* n)
        {
            //cast the old type to the child's type to compare correctly
            if (old == a)
                setChildA(n);
            else
                Node0::replaceChild(old, n);
        }

        Node0* getChildA()
        {
            return a;
        }

        void setChildA(Node0* newchld)
        {
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
            os << 'n' << static_cast<Node0*>(this) << " -> n" << a << ";\n";
            a->emitDot(os);
            Node0::emitDot(os);
        }
    };

    class Node2 : public Node1
    {
        Node0* b;

    public:
        Node2(Node0* a_i, Node0* b_i, tok::Location const &l)
            : Node1(a_i, l), b(b_i)
        {
            b->parent = this;
        }

        Node2(Node0* a_i, Node0* b_i)
            : Node1(a_i, a_i->loc + b_i->loc), b(b_i)
        {
            b->parent = this;
        }

        ~Node2()
        {
            delete b;
        }

        void eachChild(sa::AstWalker0 *w)
        {
            Node1::eachChild(w);
            w->call(b);
        }

        void replaceChild(Node0* old, Node0* n)
        {
            //cast the old type to the child's type to compare correctly
            if (old == b)
                setChildB(n);
            else
                Node1::replaceChild(old, n);
        }

        Node0* getChildB()
        {
            return b;
        }

        void setChildB(Node0* newchld)
        {
            b = newchld;
            newchld->parent = this;
        }

        void nullChildB()
        {
            b = 0;
        }

        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<Node0*>(this) << " -> n" << b << ";\n";
            b->emitDot(os);
            Node1::emitDot(os);
        }
    };

    class Node3 : public Node2
    {
        Node0* c;

    public:
        Node3(Node0* a_i, Node0* b_i, Node0* c_i, tok::Location const &l)
            : Node2(a_i, b_i, l), c(c_i)
        {
            c->parent = this;
        }

        Node3(Node0* a_i, Node0* b_i, Node0* c_i)
            : Node2(a_i, b_i, a_i->loc + c_i->loc), c(c_i)
        {
            c->parent = this;
        }

        ~Node3()
        {
            delete c;
        }

        void eachChild(sa::AstWalker0 *w)
        {
            Node2::eachChild(w);
            w->call(c);
        }

        void replaceChild(Node0* old, Node0* n)
        {
            //cast the old type to the child's type to compare correctly
            if (old == c)
                setChildC(n);
            else
                Node2::replaceChild(old, n);
        }

        Node0* getChildC()
        {
            return c;
        }

        void setChildC(Node0* newchld)
        {
            c = newchld;
            newchld->parent = this;
        }

        void nullChildC()
        {
            c = 0;
        }

        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<Node0*>(this) << " -> n" << c << ";\n";
            c->emitDot(os);
            Node2::emitDot(os);
        }
    };

    //node with any number of children
    struct NodeN : public Node0
    {
    private:
        typedef std::vector<Node0*> conts_t;
        conts_t chld;
        
    public:
        NodeN() : Node0(tok::Location()) {}
        NodeN(tok::Location const &l) : Node0(l) {}
        NodeN(Node0* first, tok::Location const &l)
            : Node0(l)
        {
            appendChild(first);
        }

        ~NodeN()
        {
            for (auto n : chld)
                delete n;
        };

        void eachChild(sa::AstWalker0 *w)
        {
            for (auto n : chld)
                w->call(n);
        }

        void replaceChild(Node0* old, Node0* nw)
        {
            for (Node0*& n : chld) //specify ref type to be able to modify it
                if (n == old)
                {
                    n = nw;
                    return;
                }
            assert(false && "didn't find child");
        }

        Node0*& getChild(int n)
        {
            return chld[n];
        }

        const conts_t& Children() const
        {
            return chld;
        }

        void setChild(int n, Node0* c)
        {
            c->parent = this;
            chld[n] = c;
        }

        void appendChild(Node0* c)
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
                os << 'n' << static_cast<Node0*>(this) << ":p" << p <<  " -> n" << static_cast<Node0*>(n) << ";\n";
                ++p;
                n->emitDot(os);
            }
            os << 'n' << static_cast<Node0*>(this) << " [label=\"" << myLbl();
            for (unsigned int p = 0; p < chld.size(); ++p)
                os << "|<p" << p << ">     ";
            os << "\",shape=record,style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }

        void consume(NodeN* bb)
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
        std::tuple<NewNode*, Node0*, NewNode*>
        split(typename conts_t::const_iterator posn)
        {
            NewNode* right = new NewNode();

            right->chld.insert(right->chld.begin(), posn + 1, chld.cend());
            chld.erase(chld.begin() + (posn - chld.cbegin()) + 1, chld.end());

            for (auto c : right->chld)
                c->parent = right;

            Node0* middle = chld.back();
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
