#include "Type.h"
#include "Util.h"
#include "Error.h"
#include "CompUnit.h"
#include "Parser.h"

#include <list>
#include <algorithm>

using namespace par;
using namespace typ;

namespace typ
{

TypeCompareResult TypeCompareResult::valid(true);
TypeCompareResult TypeCompareResult::invalid(false);
    
struct TypeNodeB
{
    //standard fuzzy type comparison
    virtual TypeCompareResult compare (TypeNodeB* other) = 0;

    //exact comparison with param substitution used when inserting
    virtual bool insertCompare (TypeNodeB* toAdd) = 0;

    virtual TypeNodeB* clone(TypeManager*) = 0;

    virtual void print(std::ostream&) = 0;

    virtual ~TypeNodeB() {};
};

template <class T>
struct TypeNode : public TypeNodeB
{
    TypeCompareResult compare (TypeNodeB* other);
    bool insertCompare (TypeNodeB* toAdd);
};

//CRTP FTW
struct FuncNode : public TypeNode<FuncNode>
{
    TypeNodeB *ret, *arg;
    TypeCompareResult compareTo(FuncNode* other);
    bool insertCompareTo(FuncNode* other);
    TypeNodeB* clone(TypeManager*);
    void print(std::ostream&);
};

struct ListNode : public TypeNode<ListNode>
{
    int length;
    TypeNodeB* contents;
    TypeCompareResult compareTo(ListNode* other);
    bool insertCompareTo(ListNode* other);
    TypeNodeB* clone(TypeManager*);
    void print(std::ostream&);
};

struct TupleNode : public TypeNode<TupleNode>
{
    std::list<std::pair<TypeNodeB*, ast::Ident>> conts;
    TypeCompareResult compareTo(TupleNode* other);
    bool insertCompareTo(TupleNode* other);
    TypeNodeB* clone(TypeManager*);
    void print(std::ostream&);
};

struct RefNode : public TypeNode<RefNode>
{
    TypeNodeB* contents;
    TypeCompareResult compareTo(RefNode* other);
    bool insertCompareTo(RefNode* other);
    TypeNodeB* clone(TypeManager*);
    void print(std::ostream&);
};

//this represents the USE of a named type
struct NamedNode : public TypeNode<NamedNode>
{
    //the real type with all args subbed in. what it "really is."
    TypeNodeB* type;
    ast::Ident name;
    //all of the args to the typedef
    std::map<ast::Ident, TypeNodeB*> args;
    TypeCompareResult compareTo(NamedNode*);
    bool insertCompareTo(NamedNode* other);
    TypeNodeB* clone(TypeManager*);
    void print(std::ostream&);
};

struct ParamNode : public TypeNode<ParamNode>
{
    ast::Ident name;
    TypeNodeB* sub; //the type this temporarily represents while substituting
    TypeCompareResult compareTo(ParamNode* other);
    bool insertCompareTo(ParamNode* other);
    TypeNodeB* clone(TypeManager*);
    void print(std::ostream&);
    ParamNode() : sub(0) {}
};

struct PrimitiveNode : public TypeNode<PrimitiveNode>
{
    const char * name;
    PrimitiveNode(const char * n) : name(n) {}
    TypeCompareResult compareTo(PrimitiveNode* other) {return this == other;}
    bool insertCompareTo(PrimitiveNode* other) {return this == other;}
    TypeNodeB* clone(TypeManager*) {return this;}
    void print(std::ostream& os) {os << name;}
};

//get a version of this with temporary param subs
//return true if it did something
bool sub(TypeNodeB*& node)
{
    ParamNode* pn = dynamic_cast<ParamNode*>(node);
    if (pn && pn->sub)
    {
        node = pn->sub;
        return true;
    }
    else
        return false;
}

//also strip names
TypeCompareResult dename(TypeNodeB*& node)
{
    if (NamedNode* realNode = dynamic_cast<NamedNode*>(node))
    {
        node = realNode->type;
        return TypeCompareResult::valid + 1;
    }
    if (TupleNode* realNode = dynamic_cast<TupleNode*>(node))
    {
        //conversion from [x] to x is considered denaming
        //TODO: should it be "free" or incur a penalty?
        if (realNode->conts.size() == 1)
        {
            node = realNode->conts.front().first;
            return TypeCompareResult::valid + 1;
        }
    }
    return TypeCompareResult::valid;
}

template<class T>
TypeCompareResult TypeNode<T>::compare (TypeNodeB* other)
{
    if (this == other) //early out!
        return TypeCompareResult::valid;
        
    TypeNodeB* realThis = this;
        
    //only call compare again if either function did something, otherwise fall through
    TypeCompareResult penalty = dename(realThis) + dename(other);
    if (penalty != TypeCompareResult::valid)
        return realThis->compare(other) + penalty;

    if (T* otherT = dynamic_cast<T*>(other)) //see if it's one of us
        return static_cast<T*>(this)->compareTo(otherT); //keep comparing
    else
        return TypeCompareResult::invalid;
}

template<class T>
bool TypeNode<T>::insertCompare (TypeNodeB* toAdd)
{
    //don't call insertCompare again, in case type is self referential ie
    // type foo?T = ?T;
    // type bar?T = foo!T;
    // ?T is set to sub for itself which is entirely valid, but it should stop there
    //only sub in toAdd because we don't want to mess with this
    //sub before doing equality comparison otherwise params will not work
    sub(toAdd);

    //don't early out
    //only call compare again if either function did something, otherwise fall through
    TypeNodeB* realThis = this;

    if (T* toAddT = dynamic_cast<T*>(toAdd)) //see if it's one of us
        return static_cast<T*>(realThis)->insertCompareTo(toAddT); //keep comparing
    else
        return false;
}

TypeCompareResult FuncNode::compareTo(FuncNode* other)
{
    return ret->compare(other->ret) + arg->compare(other->arg);
}

bool FuncNode::insertCompareTo(FuncNode* other)
{
    return ret->insertCompare(other->ret) && arg->insertCompare(other->arg);
}

TypeNodeB* FuncNode::clone(TypeManager* mgr)
{
    FuncNode* copy = new FuncNode();
    copy->ret = mgr->clone(ret);
    copy->arg = mgr->clone(arg);
    return copy;
}

void FuncNode::print(std::ostream &out)
{
    ret->print(out);
    out << ':';
    arg->print(out);
}

TypeCompareResult ListNode::compareTo(ListNode* other)
{
    return contents->compare(other);
}

bool ListNode::insertCompareTo(ListNode* other)
{
    return contents->insertCompare(other->contents);
}

TypeNodeB* ListNode::clone(TypeManager* mgr)
{
    ListNode* copy = new ListNode(*this);
    copy->contents = mgr->clone(contents);
    return copy;
}

void ListNode::print(std::ostream &out)
{
    out << "{ ";
    contents->print(out);
    out << " }";
}

TypeCompareResult TupleNode::compareTo(TupleNode* other)
{
    if (other->conts.size() != conts.size())
        return TypeCompareResult::invalid;

    TypeCompareResult ret = TypeCompareResult::valid;
    auto myit = conts.begin();
    auto otherit = other->conts.begin();
    for (; myit != conts.end(); ++myit, ++otherit) //only need to check one
    {
        if (myit->second != otherit->second)
            ret += 1; //this is a de-naming, i guess?
        ret += myit->first->compare(otherit->first);
    }
    return ret;
}

bool TupleNode::insertCompareTo(TupleNode* other)
{
    if (other->conts.size() != conts.size())
        return false;

    auto myit = conts.begin();
    auto otherit = other->conts.begin();
    for (; myit != conts.end(); ++myit, ++otherit) //only need to check one
    {
        if (myit->second != otherit->second)
            return false;
        if (!myit->first->insertCompare(otherit->first))
            return false;
    }
    return true;
}

TypeNodeB* TupleNode::clone(TypeManager* mgr)
{
    TupleNode* copy = new TupleNode();
    for (auto t : conts)
        copy->conts.emplace_back(mgr->clone(t.first), t.second);
    return copy;
}

void TupleNode::print(std::ostream &out)
{
    out << '[';
    for (auto it = conts.begin(); it != conts.end();)
    {
        it->first->print(out);
        if (++it != conts.end())
            out << ", ";
    }
    out << ']';
}

TypeCompareResult RefNode::compareTo(RefNode* other)
{
    return contents->compare(other);
}

bool RefNode::insertCompareTo(RefNode* other)
{
    return contents->compare(other);
}

TypeNodeB* RefNode::clone(TypeManager* mgr)
{
    RefNode* copy = new RefNode();
    copy->contents = mgr->clone(contents);
    return copy;
}

void RefNode::print(std::ostream &out)
{
    out << '@';
    contents->print(out);
}

TypeCompareResult NamedNode::compareTo(NamedNode* other)
{
    return insertCompareTo(other); //dummy
}

bool NamedNode::insertCompareTo(NamedNode* other)
{
    //don't bother doing fuzzy compares, it's only used in insertcompares
    return name == other->name
            && type == other->type
            && args.size() == other->args.size()
            && std::equal(args.begin(), args.end(), other->args.begin());
}

TypeNodeB* NamedNode::clone(TypeManager* mgr)
{
    NamedNode* copy = new NamedNode();
    for (auto t : args)
        copy->args[t.first] = mgr->clone(t.second);
    copy->name = name;
    copy->type = mgr->clone(type);
    return copy;
}

void NamedNode::print(std::ostream &out)
{
    out << name;
    if (!args.size())
        return;

    out << "!(";
    for (auto it = args.begin(); it != args.end();)
    {
        it->second->print(out);
        if (++it != args.end())
            out << ", ";
    }
    out << ')';
}

TypeCompareResult ParamNode::compareTo(ParamNode* other)
{
    return name == other->name;
}

bool ParamNode::insertCompareTo(ParamNode* other)
{
    return name == other->name;
}

TypeNodeB* ParamNode::clone(TypeManager*)
{
    assert(sub && "sub should be set here");
    return sub;
}

void ParamNode::print(std::ostream &out)
{
    out << '?' << name;
}

//get the node of this type (if it is that type) behind any typedefs
//this is used to construct type category specific Type subclasses
template<class T>
T* underlying_node(TypeNodeB* curNode)
{
    NamedNode* nn;
    //iterate past all named nodes
    while ((nn = dynamic_cast<NamedNode*>(curNode)) != 0)
    {
        curNode = nn->type;
    }
    //cast to desired type
    return dynamic_cast<T*>(curNode);
}

PrimitiveNode nint8("int!8");
PrimitiveNode nint16("int!16");
PrimitiveNode nint32("int!32");
PrimitiveNode nint64("int!64");
    
PrimitiveNode nfloat16("float!16");
PrimitiveNode nfloat32("float!32");
PrimitiveNode nfloat64("float!64");
PrimitiveNode nfloat80("float!80");

PrimitiveNode nany("?");
PrimitiveNode nnull("void");
PrimitiveNode noverload("<overload group>");
PrimitiveNode nerror("<error type>");

Type int8(nint8);
Type int16(nint16);
Type int32(nint32);
Type int64(nint64);

Type float16(nfloat16);
Type float32(nfloat32);
Type float64(nfloat64);
Type float80(nfloat80);

Type any(nany);
Type null(nnull);
Type overload(noverload);
Type error(nerror);

Type::Type()
    : node(&nnull)
{}

TypeCompareResult Type::compare(const Type& other)
{
    return node->compare(other.node);
}

std::string Type::to_str()
{
    std::stringstream strstr;
    node->print(strstr);
    return strstr.str();
}

//make our lives a little easer with a macro
#define ListSubGetter(T) \
T ## Type Type::get ## T() \
{ \
    T ## Type ret; \
    ret.node = ret.und_node = underlying_node<T ## Node>(node); \
    return ret; \
}

ListSubGetter(Func);
ListSubGetter(List);
ListSubGetter(Tuple);
ListSubGetter(Ref);
ListSubGetter(Named);
ListSubGetter(Param);
ListSubGetter(Primitive);

Type FuncType::arg() {return und_node->arg;}
Type FuncType::ret() {return und_node->ret;}

TypeManager::~TypeManager()
{
    for (auto it : nodes)
        delete it;
}

Type TypeManager::makeList(Type conts, int length)
{
    ListNode* n = new ListNode();
    n->contents = conts;
    n->length = length;
    return unique(n);
}

Type TypeManager::makeRef(Type conts)
{
    RefNode* n = new RefNode();
    n->contents = conts;
    return unique(n);
}

Type TypeManager::finishNamed(Type conts, ast::Ident name)
{
    NamedNode* n = new NamedNode();
    n->name = name;
    std::swap(n->args, namedConts);
    n->type = substitute(conts, n->args);
    return unique(n); //we still need to unique n, n != n->type
}

Type TypeManager::makeParam(ast::Ident name)
{
    ParamNode* n = new ParamNode();
    n->name = name;
    return unique(n);
}

Type TypeManager::makeFunc(Type ret, Type arg)
{
    FuncNode* n = new FuncNode();
    n->ret = ret;
    //function args are always tuples
    if (arg.getTuple().isValid())
        n->arg = arg;
    else
    {
        addToTuple(arg, 0); //is null ident == 0 a safe assumption?
        n->arg = finishTuple();
    }
    return unique(n);
}

Type TypeManager::finishTuple()
{
    TupleNode* n = new TupleNode();
    std::swap(n->conts, tupleConts); //handily clears out tupleConts
    return unique(n);
}

TypeNodeB* TypeManager::unique(TypeNodeB* n)
{
    //primitive nodes are always unique
    if (dynamic_cast<PrimitiveNode*>(n))
        return n;

    for (auto it : nodes)
    {
        if (it->insertCompare(n))
        {
            if (it != n) //don't delete things we already own
                delete n;
            return it;
        }
    }

    nodes.push_back(n);
    return n;
}

//ok here's how this works. whenever a named type is used, we run this fuction
//on its contents, which creates (if neccesary) a new node which represents the type with
//exactly these arguments. we do this by temporarily replacing parameter nodes
//with the values that they represent, then making a copy of and re-uniqing
//all nodes below it. we can do this without interfering with other types because
//we will at this point already have nodes for any contained types.
//note that we have to be careful with the old node, because things are pointing to it
Type TypeManager::substitute(Type old, std::map<ast::Ident, TypeNodeB*>& subs)
{
    //first set parameter subs
    for (auto node : nodes)
        if (ParamNode* pn = dynamic_cast<ParamNode*>(node))
            if (subs.count(pn->name))
                pn->sub = subs[pn->name];


    TypeNodeB* newNode = clone(old);

    //finally clear parameter subs
    for (auto node : nodes)
        if (ParamNode* pn = dynamic_cast<ParamNode*>(node))
            pn->sub = 0;
        
    return newNode;
}

TypeNodeB* TypeManager::clone(TypeNodeB* n)
{
    return unique(n->clone(this));
}

}
