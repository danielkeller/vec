#ifndef ASTNODE_H
#define ASTNODE_H

#include "Token.h"
#include "Type.h"

#include <functional>
#include <ostream>
#include <cassert>
#include <list>
#include <vector>
#include <memory>

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
    struct deleter {inline void operator()(Node0* x);};
    template<class Node>
    struct NPtr
    {
        typedef std::unique_ptr<Node, deleter> type;
    };
    typedef std::unique_ptr<Node0, deleter> Ptr;
    
    //convenience method to created a Node Ptr using TAD
    template<class Node>
    typename NPtr<Node>::type MkNPtr(Node* ptr)
    {
        return (typename NPtr<Node>::type)(ptr);
    }
    //the gist of this is: you can't just make and delete ast nodes. they must be created and
    //passed around inside unique_ptrs. if you want a "reference" to the node, you can get one
    //in the form of a normal pointer, but you can't delete it or change its attachment

    //to be fully safe, all classes should use factory constructors that return unique_ptrs
    //however, this requires a lot of boilerplate code.

    //when/if MSVC supports variadic templates, each class should have a private c'tor & d'tor
    //and be friends with a function similar to
    /*
    template<class T, class...Args>
    std::unique_ptr<T> make_unique(Args&&... args)
    {
        std::unique_ptr<T> ret (new T(std::forward<Args>(args)...));
        return ret;
    }
    */

    //virtual Node base class for generic node classes ie expr, stmt

    class Node0
    {
    protected:
        //type is private, and has a virtual "sgetter" so we can override its behavior
        //when a derived Expr has some sort of deterministic type. wasted space isn't
        //a big deal since it's only sizeof(void*) bytes
        typ::Type type;

        //facility for creating unique_ptrs to ast node classes
        Node0(tok::Location const &l) : loc(l), parent(0) {};
        virtual ~Node0() {};
        friend struct deleter;

        void setParent(const Ptr& node)
        {
            if (node)
                node->parent = this;
        }

        void nullParent(const Ptr& node)
        {
            if (node)
                node->parent = nullptr;
        }

        //this allows emitDot to be robust in case of null nodes
        void nodeDot(const Ptr& n, std::ostream &os)
        {
            if (n)
                n->emitDot(os);
        }
        void nodeDot(Node0* n, std::ostream &os)
        {
            if (n)
                n->emitDot(os);
        }

    public:
        virtual typ::Type& Type() {return type;} //sgetter. sget it?
        virtual bool isLval() {return false;}
        virtual bool isExpr() {return true;}

        tok::Location loc; //might not be set
        Node0 *parent;

        virtual void eachChild(sa::AstWalker0*) {};

        virtual void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<Node0*>(this) << " [label=\"" << myLbl()
                << "\",style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }

        //replace child of unknown position
        virtual Ptr replaceChild(Node0*, Ptr)
        {
            assert(false && "didn't find child");
            return nullptr;
        }

        //replace child that was detached (and so set to null)
        Ptr replaceDetachedChild(Ptr n)
        {
            return replaceChild(nullptr, move(n));
        }

        Ptr detachSelf()
        {
            return parent->replaceChild(this, nullptr);
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
        Ptr a;

    protected:
        Node1(Ptr a_i, tok::Location const &l)
            : Node0(l), a(move(a_i))
        {
            setParent(a);
        }

        Node1(Ptr a_i)
            : Node0(a_i->loc), a(move(a_i))
        {
            setParent(a);
        }

        ~Node1()
        {
        }

    public:

        void eachChild(sa::AstWalker0 *w)
        {
            w->call(getChildA());
        }

        Ptr replaceChild(Node0* old, Ptr n)
        {
            //cast the old type to the child's type to compare correctly
            if (old == getChildA())
                return setChildA(move(n));
            else
                return Node0::replaceChild(old, move(n));
        }

        Node0* getChildA()
        {
            return a.get();
        }

        Ptr detachChildA()
        {
            nullParent(a);
            return move(a);
        }

        template<class Node>
        typename NPtr<Node>::type detachChildAAs()
        {
            if (exact_cast<Node*>(getChildA()))
                return NPtr<Node>::type(static_cast<Node*>(detachChildA().release()));
            else
                return NPtr<Node>::type();
        }

        Ptr setChildA(Ptr newchld)
        {
            std::swap(a, newchld);
            setParent(a);
            return move(newchld);
        }

        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<Node0*>(this) << " -> n" << getChildA() << ";\n";
            nodeDot(a, os);
            Node0::emitDot(os);
        }
    };

    class Node2 : public Node1
    {
        Ptr b;

    protected:
        Node2(Ptr a_i, Ptr b_i, tok::Location const &l)
            : Node1(move(a_i), l), b(move(b_i))
        {
            setParent(b);
        }

        Node2(Ptr a_i, Ptr b_i)
            : Node1(move(a_i), a_i->loc + b_i->loc), b(move(b_i))
        {
            setParent(b);
        }

        ~Node2()
        {
        }

    public:
        void eachChild(sa::AstWalker0 *w)
        {
            Node1::eachChild(w);
            w->call(getChildB());
        }

        Ptr replaceChild(Node0* old, Ptr n)
        {
            //cast the old type to the child's type to compare correctly
            if (old == getChildB())
                return setChildB(move(n));
            else
                return Node1::replaceChild(old, move(n));
        }

        Node0* getChildB()
        {
            return b.get();
        }

        Ptr detachChildB()
        {
            nullParent(b);
            return move(b);
        }

        template<class Node>
        typename NPtr<Node>::type detachChildBAs()
        {
            if (exact_cast<Node*>(getChildB()))
                return NPtr<Node>::type(static_cast<Node*>(detachChildB().release()));
            else
                return NPtr<Node>::type();
        }

        Ptr setChildB(Ptr newchld)
        {
            std::swap(b, newchld);
            setParent(b);
            return move(newchld);
        }

        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<Node0*>(this) << " -> n" << getChildB() << ";\n";
            nodeDot(b, os);
            Node1::emitDot(os);
        }
    };

    class Node3 : public Node2
    {
        Ptr c;

    protected:
        Node3(Ptr a_i, Ptr b_i, Ptr c_i, tok::Location const &l)
            : Node2(move(a_i), move(b_i), l), c(move(c_i))
        {
            setParent(c);
        }

        Node3(Ptr a_i, Ptr b_i, Ptr c_i)
            : Node2(move(a_i), move(b_i), a_i->loc + c_i->loc), c(move(c_i))
        {
            setParent(c);
        }

        ~Node3()
        {
        }

    public:
        void eachChild(sa::AstWalker0 *w)
        {
            Node2::eachChild(w);
            w->call(getChildC());
        }

        Ptr replaceChild(Node0* old, Ptr n)
        {
            //cast the old type to the child's type to compare correctly
            if (old == getChildC())
                return setChildC(move(n));
            else
                return Node2::replaceChild(old, move(n));
        }

        Node0* getChildC()
        {
            return c.get();
        }

        Ptr detachChildC()
        {
            nullParent(c);
            return move(c);
        }

        template<class Node>
        typename NPtr<Node>::type detachChildCAs()
        {
            if (exact_cast<Node*>(getChildC()))
                return NPtr<Node>::type(static_cast<Node*>(detachChildC().release()));
            else
                return NPtr<Node>::type();
        }

        Ptr setChildC(Ptr newchld)
        {
            std::swap(c, newchld);
            setParent(c);
            return move(newchld);
        }

        void emitDot(std::ostream &os)
        {
            os << 'n' << static_cast<Node0*>(this) << " -> n" << getChildC() << ";\n";
            nodeDot(c, os);
            Node2::emitDot(os);
        }
    };

    //node with any number of children
    struct NodeN : public Node0
    {
    private:
        typedef std::vector<Ptr> conts_t;
        conts_t chld;
        
    protected:
        NodeN() : Node0(tok::Location()) {}
        NodeN(tok::Location const &l) : Node0(l) {}
        NodeN(Ptr first, tok::Location const &l)
            : Node0(l)
        {
            appendChild(move(first));
        }

        ~NodeN()
        {
        };

    public:
        void eachChild(sa::AstWalker0 *w)
        {
            for (Ptr& c : chld) //have to use ref type to not make a copy
                w->call(c.get());
        }

        Ptr replaceChild(Node0* old, Ptr n)
        {
            for (Ptr& c : chld)
                if (c.get() == old)
                {
                    std::swap(c, n);
                    setParent(c);
                    return move(n);
                }
            assert(false && "didn't find child");
            return nullptr;
        }

        Node0* getChild(int n)
        {
            return chld[n].get();
        }

        Ptr detachChild(int n)
        {
            nullParent(chld[n]);
            return move(chld[n]);
        }

        Ptr detachChild(conts_t::const_iterator posn)
        {
            //we have to do this weird syntax because iterators aren't very unique_ptr friendly
            return detachChild(posn - chld.begin());
        }

        template<class Node>
        typename NPtr<Node>::type detachChildAs(int n)
        {
            if (exact_cast<Node*>(getChild(n)))
                return MkNPtr(static_cast<Node*>(detachChild(n).release()));
            else
                return nullptr;
        }

        template<class Node>
        typename NPtr<Node>::type detachChildAs(conts_t::const_iterator posn)
        {
            if (exact_cast<Node*>(*posn))
                return MkNPtr(static_cast<Node*>(detachChild(posn).release()));
            else
                return nullptr;
        }


        Ptr setChild(int n, Ptr newchild)
        {
            std::swap(chld[n], newchild);
            setParent(chld[n]);
            return move(newchild);
        }

        //TODO: this is bad coding; it exposes implementation details
        const conts_t& Children() const
        {
            return chld;
        }

        void appendChild(Ptr c)
        {
            setParent(c);
            chld.push_back(move(c));
        }

        void popChild()
        {
            chld.pop_back();
        }

        void emitDot(std::ostream &os)
        {
            int p = 0;
            for (Ptr& n : chld)
            {
                os << 'n' << static_cast<Node0*>(this) << ":p" << p <<  " -> n" << n.get() << ";\n";
                ++p;
                nodeDot(n, os);
            }
            os << 'n' << static_cast<Node0*>(this) << " [label=\"" << myLbl();
            for (unsigned int p = 0; p < chld.size(); ++p)
                os << "|<p" << p << ">     ";
            os << "\",shape=record,style=filled,fillcolor=\"/pastel19/" << myColor() << "\"];\n";
        }

        void swap(NodeN* other)
        {
            std::swap(chld, other->chld);
            for (Ptr& c : chld)
                setParent(c);
        }

        void consume(NPtr<NodeN>::type other)
        {
            for (Ptr& c : other->chld)
                appendChild(move(c));
        }

        //split this node in half at the position of the iterator
        //left < posn, middle == posn, right > posn.
        template<class NewNode>
        std::tuple<typename NPtr<NewNode>::type, Ptr, typename NPtr<NewNode>::type>
        split(conts_t::const_iterator posn)
        {
            assert(parent && "split may only be called on attached child");

            auto right = MkNPtr(new NewNode());

            for (auto it = posn + 1; it < chld.cend(); ++it)
                right->appendChild(detachChild(it));
            //MSVC doesn't support C++11 erase yet
            chld.erase(chld.begin() + (posn - chld.cbegin()) + 1, chld.end());

            for (Ptr& c : right->chld)
                right->setParent(c);

            Ptr middle = move(chld.back());
            chld.pop_back();

            auto left = MkNPtr(new NewNode());
            left->swap(this);

            return std::make_tuple(move(left), move(middle), move(right));
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
