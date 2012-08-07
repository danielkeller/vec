#include "Value.h"
#include <vector>

namespace val{

Value fromNode(ValNode* n)
{
    Value ret;
    ret.node = std::shared_ptr<ValNode>(n);
    return ret;
};

struct ValNode
{
    typ::Type type;
    virtual ValNode* clone() = 0;
};

struct NullNode : public ValNode
{
    enum k
    {
        Error,
        RunTime
    } kind;

    ValNode* clone() {return new NullNode(*this);}
};

struct FuncNode : public ValNode
{
    ast::FunctionDef* fd;

    //FIXME: should be able to clone, for functional stuff
    ValNode* clone() {return this;}
};

struct ScalarNode : public ValNode
{
    virtual byteVec& Get() = 0;
};

struct NormalScalarNode : public ScalarNode
{
    byteVec val;
    byteVec& Get() {return val;}
    ValNode* clone() {return new NormalScalarNode(*this);}
};

struct Level1SeqNode;

//reference into Level1SeqNode
struct ScalarRefNode : public ScalarNode
{
    std::shared_ptr<Level1SeqNode> memOwner; //ensure it doesn't get deleted
    size_t offset;

    ScalarRefNode(Value& v, size_t o);

    byteVec& Get();

    ValNode* clone()
    {
        NormalScalarNode* ret = new NormalScalarNode;
        ret->Get() = Get();
        ret->type = type;
        return ret;
    }
};

//lists and tuples
struct SeqNode : public ValNode
{
    virtual Value get(size_t pos, Value& me) = 0;
    virtual void append(Value v) = 0;
    virtual ~SeqNode() {}
};

//lists & tuples of scalars. owns the memory
struct Level1SeqNode : public SeqNode
{
    //use array for an 8 byte chunk of memory. we can take a lot of safety liberties here
    //because Sema already type checks everything
    std::vector<byteVec> val;

    Value get(size_t pos, Value& me)
    {
        if (type.getList().isValid())
        {
            ValNode * n = new ScalarRefNode(me, pos);
            n->type = type.getList().conts();
            return fromNode(n);
        }
        else
        {
            ValNode * n = new ScalarRefNode(me, pos);
            n->type = type.getTuple().elem(pos);
            return fromNode(n);
        }
    }

    void append(Value v)
    {
        val.push_back(static_cast<ScalarNode*>(v.node.get())->Get());
    }

    ValNode* clone() {return new Level1SeqNode(*this);}
};

ScalarRefNode::ScalarRefNode(Value& v, size_t o)
    : memOwner(v.node, static_cast<Level1SeqNode*>(v.node.get())),
    offset(o)
{}

byteVec& ScalarRefNode::Get()
{
    return memOwner->val[offset];
}

//lists & tuples of anthing. does not own the memory.
struct LevelNSeqNode : public SeqNode
{
    std::vector<Value> val;
    Value get(size_t pos, Value&)
    {
        return val[pos];
    }

    void append(Value v)
    {
        val.push_back(v);
    }

    ValNode* clone()
    {
        LevelNSeqNode *ret = new LevelNSeqNode();
        for (auto v : val)
            ret->append(fromNode(v.node->clone()));
        return ret;
    }
};

struct RefNode : public ValNode
{
    Value target;
    ValNode* clone() {return new RefNode(*this);}
};

typ::Type& Value::Type()
{
    return node->type;
}

//create a copy that can be modifed independently
Value Value::Duplicate()
{
    return fromNode(node->clone());
}

//LLVMValue getLLVMValue();

Value& Value::getRefTarget()
{
    return static_cast<RefNode*>(node.get())->target;
}

void Value::setRefTarget(Value& v)
{
    static_cast<RefNode*>(node.get())->target = v;
}

Value Value::getSeqElem(size_t pos)
{
    return static_cast<SeqNode*>(node.get())->get(pos, *this);
}

void Value::appendSeqElem(const Value& v)
{
    static_cast<SeqNode*>(node.get())->append(v);
}

byteVec& Value::getBytes()
{
    return static_cast<ScalarNode*>(node.get())->Get();
}

ast::FunctionDef*& Value::getFunc()
{
    return static_cast<FuncNode*>(node.get())->fd;
}

Value makeScalar()
{
    Value ret;
    ret.node = std::make_shared<NormalScalarNode>();
    return ret;
}

Value seq(typ::Type t)
{
    Value ret;
    ret.node = std::make_shared<LevelNSeqNode>();
    ret.Type() = t;
    return ret;
}

Value scalarSeq(typ::Type t)
{
    Value ret;
    ret.node = std::make_shared<Level1SeqNode>();
    ret.Type() = t;
    return ret;
}

}