#include "Type.h"
#include "Util.h"
#include "Error.h"
#include "Module.h"
#include "Parser.h"
#include "Global.h"
#include "LLVM.h"

#include <list>
#include <algorithm>

using namespace par;
using namespace typ;

namespace typ
{

TypeManager mgr;

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

    //we could have some fancy stuff with virtual functions, but the only type that
    //needs to be "special" is function
    llvm::Type* llvm_t;

    //requires all referenced nodes to have complete type
    virtual void createLLVMType() = 0;
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
    void createLLVMType();
};

struct ListNode : public TypeNode<ListNode>
{
    int length;
    TypeNodeB* contents;
    TypeCompareResult compareTo(ListNode* other);
    bool insertCompareTo(ListNode* other);
    TypeNodeB* clone(TypeManager*);
    void print(std::ostream&);
    void createLLVMType();
};

struct TupleNode : public TypeNode<TupleNode>
{
    std::vector<std::pair<TypeNodeB*, Ident>> conts;
    TypeCompareResult compareTo(TupleNode* other);
    bool insertCompareTo(TupleNode* other);
    TypeNodeB* clone(TypeManager*);
    void print(std::ostream&);
    void createLLVMType();
};

struct RefNode : public TypeNode<RefNode>
{
    TypeNodeB* contents;
    TypeCompareResult compareTo(RefNode* other);
    bool insertCompareTo(RefNode* other);
    TypeNodeB* clone(TypeManager*);
    void print(std::ostream&);
    void createLLVMType();
};

//this represents the USE of a named type
struct NamedNode : public TypeNode<NamedNode>
{
    //the real type with all args subbed in. what it "really is."
    TypeNodeB* type;
    Ident name;
    //all of the args to the typedef
    std::list<TypeNodeB*> args;
    TypeCompareResult compareTo(NamedNode*);
    bool insertCompareTo(NamedNode* other);
    TypeNodeB* clone(TypeManager*);
    void print(std::ostream&);
    void createLLVMType();
};

struct ParamNode : public TypeNode<ParamNode>
{
    Ident name;
    TypeNodeB* sub; //the type this temporarily represents while substituting
    TypeCompareResult compareTo(ParamNode* other);
    bool insertCompareTo(ParamNode* other);
    TypeNodeB* clone(TypeManager*);
    void print(std::ostream&);
    ParamNode() : sub(0) {}
    void createLLVMType();
};

struct PrimitiveNode : public TypeNode<PrimitiveNode>
{
    const char * name;
    PrimitiveNode(const char * n, llvm::Type* t) : name(n) {llvm_t = t;}
    PrimitiveNode(const char * n) : name(n) //for special types
    {
        llvm_t = llvm::Type::getVoidTy(llvm::getGlobalContext());
    }
    TypeCompareResult compareTo(PrimitiveNode* other) {return this == other;}
    bool insertCompareTo(PrimitiveNode* other) {return this == other;}
    TypeNodeB* clone(TypeManager*) {return this;}
    void print(std::ostream& os) {os << name;}
    void createLLVMType() {}
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
        //it is "free" so we can change tuple element names for function args
        if (realNode->conts.size() == 1)
        {
            node = realNode->conts.front().first;
            return TypeCompareResult::valid;
        }

    }
    return TypeCompareResult::valid;
}

template<class T>
TypeCompareResult TypeNode<T>::compare (TypeNodeB* other)
{
    //it is important that this happens before denaming, if they have the same name or are both [x]
    //it should not incur a penalty
    if (this == other) //early out!
        return TypeCompareResult::valid;
        
    TypeNodeB* realThis = this;

    //hey i just found you, and this is crazy, but i'm a low importance bug, so FIXME maybe?
    //denaming has to find either the lowest common ancestor in the (possibly bipartite)
    //tree of type names, or the root for each node if no such ancestor exists. consider
    //type foo = int;
    //type bar = foo;
    //type baf = foo;
    //the correct cost for bar-baf conversion is 1, but the current algo will give 2
    
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
    //only sub in toAdd because we don't want to mess with this.
    //sub before doing equality comparison otherwise params will not work
    sub(toAdd);

    //don't early out
    //TODO: can't we early out if no subs are set?
    //only call compare again if either function did something, otherwise fall through
    TypeNodeB* realThis = this;

    if (T* toAddT = exact_cast<T*>(toAdd)) //see if it's one of us
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

void FuncNode::createLLVMType()
{
    llvm::SmallVector<llvm::Type*, 5> llvm_args;
    
    TupleNode* allArgs = exact_cast<TupleNode*>(arg);
    if (allArgs)
        for (auto a : allArgs->conts)
            llvm_args.push_back(a.first->llvm_t);
    else
        llvm_args.push_back(arg->llvm_t);

    llvm_t = llvm::FunctionType::get(ret->llvm_t, llvm_args, false);
}

void FuncNode::print(std::ostream &out)
{
    ret->print(out);
    out << ':';
    arg->print(out);
}

TypeCompareResult ListNode::compareTo(ListNode* other)
{
    return contents->compare(other->contents);
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

void ListNode::createLLVMType()
{
    llvm_t = llvm::StructType::get(
        llvm::Type::getInt32Ty(llvm::getGlobalContext()),
        llvm::PointerType::getUnqual(contents->llvm_t),
        nullptr
        );
}

void ListNode::print(std::ostream &out)
{
    out << "[ ";
    contents->print(out);
    out << " ]";
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
        //different names on tuples incurs no penalty
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

void TupleNode::createLLVMType()
{
    llvm::SmallVector<llvm::Type*, 5> llvm_conts;
    for (auto t : conts)
        llvm_conts.push_back(t.first->llvm_t);
    llvm_t = llvm::StructType::get(llvm::getGlobalContext(), llvm_conts);
}

void TupleNode::print(std::ostream &out)
{
    out << '{';
    for (auto it = conts.begin(); it != conts.end();)
    {
        it->first->print(out);
        if (it->second)
            out << ' ' << it->second;
        if (++it != conts.end())
            out << ", ";
    }
    out << '}';
}

TypeCompareResult RefNode::compareTo(RefNode* other)
{
    return contents->compare(other);
}

bool RefNode::insertCompareTo(RefNode* other)
{
    return contents->compare(other).isValid();
}

TypeNodeB* RefNode::clone(TypeManager* mgr)
{
    RefNode* copy = new RefNode();
    copy->contents = mgr->clone(contents);
    return copy;
}

void RefNode::createLLVMType()
{
    llvm_t = llvm::PointerType::getUnqual(contents->llvm_t);
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
        copy->args.push_back(mgr->clone(t));
    copy->name = name;
    copy->type = mgr->clone(type);
    return copy;
}

void NamedNode::createLLVMType()
{
    llvm::Type* conts_t = type->llvm_t;
    llvm::StructType* struct_t = llvm::dyn_cast<llvm::StructType>(conts_t);

    //llvm can only make named structs
    if (!struct_t)
    {
        llvm_t = conts_t;
        return;
    }

    std::vector<llvm::Type*> type_conts;
    type_conts.reserve(struct_t->getNumElements());

    for (auto it = struct_t->element_begin(); it != struct_t->element_end(); ++it)
        type_conts.push_back(*it);

    llvm_t = llvm::StructType::create(type_conts, name);
}

void NamedNode::print(std::ostream &out)
{
    out << name;
    if (!args.size())
        return;

    out << "!(";
    for (auto it = args.begin(); it != args.end();)
    {
        (*it)->print(out);
        if (++it != args.end())
            out << ", ";
    }
    out << ")";
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

void ParamNode::createLLVMType()
{
    //this should probably not be used for anything
    llvm_t = llvm::StructType::create(llvm::getGlobalContext(), "_P_" + Global().getIdent(name));
}

void ParamNode::print(std::ostream &out)
{
    out << '?' << name;
}

PrimitiveNode nint8("int!8", llvm::Type::getInt8Ty(llvm::getGlobalContext()));
PrimitiveNode nint16("int!16", llvm::Type::getInt16Ty(llvm::getGlobalContext()));
PrimitiveNode nint32("int!32", llvm::Type::getInt32Ty(llvm::getGlobalContext()));
PrimitiveNode nint64("int!64", llvm::Type::getInt64Ty(llvm::getGlobalContext()));
    
PrimitiveNode nfloat32("float!32", llvm::Type::getFloatTy(llvm::getGlobalContext()));
PrimitiveNode nfloat64("float!64", llvm::Type::getDoubleTy(llvm::getGlobalContext()));
PrimitiveNode nfloat80("float!80", llvm::Type::getX86_FP80Ty(llvm::getGlobalContext()));

PrimitiveNode nboolean("bool", llvm::Type::getInt1Ty(llvm::getGlobalContext()));

//special types
PrimitiveNode nany("?");
PrimitiveNode nnull("void");
PrimitiveNode noverload("<overload group>");
PrimitiveNode nerror("<error type>");
PrimitiveNode nundeclared("<undefined type>");

Type int8(nint8);
Type int16(nint16);
Type int32(nint32);
Type int64(nint64);

Type float32(nfloat32);
Type float64(nfloat64);
Type float80(nfloat80);

Type boolean(nboolean);

Type any(nany);
Type null(nnull);
Type overload(noverload);
Type error(nerror);
Type undeclared(nundeclared);

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
    strstr << "'";
    node->print(strstr);
    strstr << "'";

    if (getNamed().isValid())
    {
        strstr << " aka ";
        strstr << getNamed().realType();
    }

    return strstr.str();
}

std::ostream& operator<<(std::ostream& lhs, Type& rhs)
{
    return lhs << rhs.to_str();
}

llvm::Type* Type::toLLVM()
{
    return node->llvm_t;
}

//get the node of this type (if it is that type) behind any typedefs
//this is used to construct type category specific Type subclasses
template<class T>
T* underlying_node(TypeNodeB* curNode)
{
    NamedNode* nn;
    //iterate past all named nodes. this is so we can get the list type of string for example
    while ((nn = dynamic_cast<NamedNode*>(curNode)) != 0)
    {
        curNode = nn->type;
    }
    //cast to desired type
    return exact_cast<T*>(curNode);
}

//make our lives a little easer with a macro
#define ListSubGetter(T) \
T ## Type Type::get ## T() const \
{ \
    T ## Type ret; \
    ret.node = ret.und_node = underlying_node<T ## Node>(node); \
    return ret; \
}

ListSubGetter(Func)
ListSubGetter(List)
ListSubGetter(Tuple)
ListSubGetter(Ref)
ListSubGetter(Param)
ListSubGetter(Primitive)

//don't use underlying_node for named nodes
NamedType Type::getNamed() const
{
    NamedType ret;
    ret.node = ret.und_node = exact_cast<NamedNode*>(node);
    return ret;
}

Type FuncType::arg() {return und_node->arg;}
Type FuncType::ret() {return und_node->ret;}

Type ListType::conts() {return und_node->contents;}

Type TupleType::elem(size_t pos) {return und_node->conts[pos].first;}

Ident NamedType::name() {return und_node->name;}
Type NamedType::realType() {return und_node->type;}
unsigned int NamedType::numArgs() {return und_node->args.size();}
bool NamedType::isExternal() {return und_node->type == &nundeclared;}

bool PrimitiveType::isArith()
{
    return isInt() || isFloat();
}

bool PrimitiveType::isInt()
{
    return node == &nint8
        || node == &nint16
        || node == &nint32
        || node == &nint64;
}

bool PrimitiveType::isFloat()
{
    return node == &nfloat32
        || node == &nfloat64
        || node == &nfloat80;
}

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

Type TypeManager::makeNamed(Type conts, Ident name, std::vector<Ident>& params, std::list<TypeNodeB*>& args)
{
    NamedNode* n = new NamedNode();
    n->name = name;
    std::swap(n->args, args);

    //recover as best we can. also do "aliasing" here.
    //get the name of the param and replace ie '?T' with '?T in foo'
    while (n->args.size() < params.size())
    {
        Ident alias = Global().addIdent(
            "<" + Global().getIdent(params[n->args.size()]) + " in "
            + Global().getIdent(n->name) + ">");
        n->args.push_back(typ::mgr.makeParam(alias));
    }

    std::map<Ident, TypeNodeB*> subs;
    auto it = n->args.begin();
    for (unsigned int idx = 0; idx < params.size(); ++idx)
    {
        subs[params[idx]] = *it;
        ++it;
    }

    n->type = substitute(conts, subs);
    return unique(n);
}

Type TypeManager::makeNamed(Type conts, Ident name)
{
    NamedNode* n = new NamedNode();
    n->name = name;
    n->type = conts;
    return unique(n); //we still need to unique n, n != n->type
}

Type TypeManager::makeExternNamed(Ident name, NamedBuilder& args)
{
    NamedNode* n = new NamedNode();
    n->name = name;
    std::swap(n->args, args.namedConts);
    n->type = undeclared;
    return unique(n);
}

Type TypeManager::fixExternNamed(NamedType toFix, Type conts, std::vector<Ident>& params)
{
    //create new, unique type
    NamedNode* n = static_cast<NamedNode*>(toFix.node);
    return makeNamed(conts, n->name, params, n->args);
}

Type TypeManager::makeParam(Ident name)
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
        typ::TupleBuilder builder;
        builder.push_back(arg, Global().reserved.null);
        n->arg = makeTuple(builder);
    }
    return unique(n);
}

Type TypeManager::makeTuple(TupleBuilder& builder)
{
    TupleNode* n = new TupleNode();
    std::swap(n->conts, builder.tupleConts);
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

    if (makeLLVMImmediately)
        n->createLLVMType();

    return n;
}


void TypeManager::makeLLVMTypes()
{
    //if we do this in order, all nodes will have defined types to refer to,
    //because all nodes only refer to nodes before them
    for (auto node : nodes)
        node->createLLVMType();

    makeLLVMImmediately = true;
}

void TypeManager::printAll(std::ostream& os)
{
    for (auto node : nodes)
    {
        os << Type(node).to_str() << std::endl;
    }
}

//ok here's how this works. whenever a named type is used, we run this fuction
//on its contents, which creates (if neccesary) a new node which represents the type with
//exactly these arguments. we do this by temporarily replacing parameter nodes
//with the values that they represent, then making a copy of and re-uniqing
//all nodes below it. we can do this without interfering with other types because
//we will at this point already have nodes for any contained types.
//note that we have to be careful with the old node, because things are pointing to it
Type TypeManager::substitute(Type old, std::map<Ident, TypeNodeB*>& subs)
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
