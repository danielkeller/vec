#ifndef ASTWALKER_H
#define ASTWALKER_H

#include "AstNode.h"

//support for super-easy tree iteration. syntax looks like
/*
for (auto n : Subtree<NodeType>(root))

or
for (auto n : Subtree<>(root))
for no filtering

or
for (auto n : Subtree<WeirdType, Dynamic>(root))
for dynamic casting

or
for (auto n : Subtree<NodeType>(root).cached())
for cached iteration

or
for (auto n : Subtree<NodeType>(root).preorder())
for preorder iteration
*/

namespace sa
{
    //fake enum
    namespace Cast
    {
        class Exact {};
        class Dynamic {};
    }

    namespace Order
    {
        class Pre {};
        class Post {};
    }

    //fake container class that is compatible with c++11's range-for loop
    template<class filter = ast::Node0, class castType = Cast::Exact, class order = Order::Post>
    class Subtree
    {
        class Iterator
        {
            friend class Subtree<filter, castType, order>;
            //end is needed so we don't accidentally pass it looking for something of a specific type
            ast::Node0* n;
            ast::Node0* end;

            Iterator(ast::Node0* root)
                : n(root), end(root) {}

            //can't parially specialize functions, so fake it
            filter* doCast(Cast::Exact);
            filter* doCast(Cast::Dynamic);

            //suport for pre/postorder iteration
            void init(Order::Pre); //go from root to 1st node
            void init(Order::Post);
            void next(Order::Pre); //go to next node in order
            void next(Order::Post);

            void descend(); //go down the left sub tree

            void skip();
        public:

            //public iterator interface
            Iterator& operator++();
            bool operator!=(const Iterator&) const;
            filter* operator*() {return doCast(castType());}

            //this is an inconsistency, but it's convenient
            filter* operator->() {return **this;}

            //skip over subtree below iterator
            void skipSubtree();
        };

        ast::Node0* root;

    public:
        Subtree(ast::Node0* root)
            : root(root) {}

        Iterator begin()
        {
            Iterator ret = Iterator(root);
            ret.init(order());
            ret.skip();
            return ret;
        }

        Iterator end()
        {
            //"fast-forward" it to the end
            Iterator e = begin();
            e.n = e.end;
            return e;
        }

        //switch to preorder
        Subtree<filter, castType, Order::Pre> preorder()
        {
            return Subtree<filter, castType, Order::Pre>(root);
        }

        //do a cached loop instead of a standard one
        std::vector<filter*> cached();
    };
    
    //casting functions
    template<>
    inline ast::Node0* Subtree<ast::Node0, Cast::Exact, Order::Post>::Iterator::doCast(Cast::Exact)
    {
        return n; //don't cast it
    }

    //needed twice because it can't be partially specialized
    template<>
    inline ast::Node0* Subtree<ast::Node0, Cast::Exact, Order::Pre>::Iterator::doCast(Cast::Exact)
    {
        return n;
    }

    template<class filter, class c, class o>
    filter* Subtree<filter, c, o>::Iterator::doCast(Cast::Exact)
    {
        return exact_cast<filter*>(n);
    }

    template<class filter, class c, class o>
    filter* Subtree<filter, c, o>::Iterator::doCast(Cast::Dynamic)
    {
        return dynamic_cast<filter*>(n);
    }
    
    //all unspecialized functions

    template<class f, class c, class order>
    typename Subtree<f, c, order>::Iterator& Subtree<f, c, order>::Iterator::operator++()
    {
        next(order());
        skip();
        return *this;
    }

    template<class f, class c, class o>
    void Subtree<f, c, o>::Iterator::skipSubtree()
    {
        auto it = Subtree<f, c, o>(n).end();
        if (it.n != end) //don't pass the end of what we're iterating
            n = (++it).n;
        else
            n = end; //if we're waiting for the end node, don't loop forever
    }

    //go to next match or stop at end
    template<class f, class castType, class order>
    void Subtree<f, castType, order>::Iterator::skip()
    {
        while (n != end && doCast(castType()) == 0)
            next(order());
    }

    template<class filter, class c, class o>
    inline std::vector<filter*> Subtree<filter, c, o>::cached()
    {
        std::vector<filter*> cache;
        for (auto it : *this)
            cache.push_back(it);
        return cache;
    }

    template<class f, class c, class o>
    bool Subtree<f, c, o>::Iterator::operator!=(const Iterator& other) const
    {
        return n != other.n;
    }

    template<class f, class c, class o>
    void Subtree<f, c, o>::Iterator::descend()
    {
        //while n is a parent
        while (n->childAfter(nullptr))
            n = n->childAfter(nullptr); //set n to its first child
    }

    //pre/post order support

    template<class f, class c, class o>
    void Subtree<f, c, o>::Iterator::init(Order::Post)
    {
        descend();
    }

    template<class f, class c, class o>
    void Subtree<f, c, o>::Iterator::init(Order::Pre)
    {
        //while n is a parent
        while (end->childAfter(nullptr))
        {
            //set n to its last child
            ast::Node0* temp = end->childAfter(nullptr);
            while (end->childAfter(temp))
                temp = end->childAfter(temp);
            end = temp;
        }
    }

    //postorder traversal
    template<class f, class c, class o>
    void Subtree<f, c, o>::Iterator::next(Order::Post)
    {
        ast::Node0* next = n->parent->childAfter(n);
        if (next)
        {
            n = next;
            descend();
        }
        else
            n = n->parent;
    }

    //preorder traversal
    template<class f, class c, class o>
    void Subtree<f, c, o>::Iterator::next(Order::Pre)
    {
        if (n->childAfter(nullptr)) //can descend to the left?
            n = n->childAfter(nullptr); //do it
        else
        {
            //go up until we can go right
            while (n->parent->childAfter(n) == nullptr)
            {
                n = n->parent;
                assert (n->parent && "missed end of iteration somehow");
            }
            n = n->parent->childAfter(n);
        }
    }
}

#endif
