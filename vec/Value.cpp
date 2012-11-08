#include "Value.h"
#include "Global.h"
#include "SemaNodes.h"
#include <vector>
#include <cstring>

#include "LLVM.h"

namespace val{

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
    ast::NPtr<ast::FunctionDef>::type fd;

    //FIXME: should be able to clone, for functional stuff
    ValNode* clone() {return this;}
};

struct ScalarNode : public ValNode
{
    virtual char* Get() = 0;
};

struct NormalScalarNode : public ScalarNode
{
    std::aligned_storage<sizeof(long double), std::alignment_of<long double>::value>::type val;
    char* Get() {return reinterpret_cast<char*>(&val);}
    ValNode* clone() {return new NormalScalarNode(*this);}
};

struct Level1SeqNode;

//reference into Level1SeqNode
struct ScalarRefNode : public ScalarNode
{
    std::shared_ptr<Level1SeqNode> memOwner; //ensure it doesn't get deleted
    size_t offset;

    ScalarRefNode(Value& v, size_t o);

    char* Get();

    ValNode* clone();
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
    std::vector<char> val;
    size_t width; //size of contained value

    Level1SeqNode(size_t width)
        : width(width) {}

    Value get(size_t pos, Value& me)
    {
        if (type.getList().isValid())
        {
            ValNode * n = new ScalarRefNode(me, pos);
            n->type = type.getList().conts();
            return n;
        }
        else
        {
            ValNode * n = new ScalarRefNode(me, pos);
            n->type = type.getTuple().elem(pos);
            return n;
        }
    }

    void append(Value v)
    {
        for (size_t n = 0; n < width; ++n)
            val.push_back(0);

        memcpy(&val[val.size() - width], dynamic_cast<ScalarNode*>(v.node.get())->Get(), width);
    }

    ValNode* clone() {return new Level1SeqNode(*this);}
};

ScalarRefNode::ScalarRefNode(Value& v, size_t o)
    : memOwner(v.node, static_cast<Level1SeqNode*>(v.node.get())),
    offset(o)
{}

char* ScalarRefNode::Get()
{
    return &memOwner->val[offset * memOwner->width];
}

ValNode* ScalarRefNode::clone()
{
    NormalScalarNode* ret = new NormalScalarNode;
    memcpy(ret->Get(), Get(), memOwner->width);
    ret->type = type;
    return ret;
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
            ret->append(v.node->clone());
        return ret;
    }
};

struct RefNode : public ValNode
{
    Value target;
    ValNode* clone() {return new RefNode(*this);}
};

//create a copy that can be modifed independently
Value Value::Duplicate()
{
    return node->clone();
}

//LLVMValue getLLVMValue();

Value& Value::getRefTarget()
{
    return exact_cast<RefNode*>(node.get())->target;
}

void Value::setRefTarget(Value& v)
{
    exact_cast<RefNode*>(node.get())->target = v;
}

Value Value::getSeqElem(size_t pos)
{
    return dynamic_cast<SeqNode*>(node.get())->get(pos, *this);
}

void Value::appendSeqElem(const Value& v)
{
    dynamic_cast<SeqNode*>(node.get())->append(v);
}

char* Value::getBytes() const
{
    return dynamic_cast<ScalarNode*>(node.get())->Get();
}

ast::NPtr<ast::FunctionDef>::type& Value::getFunc()
{
    return exact_cast<FuncNode*>(node.get())->fd;
}

Value::Value(ValNode* n)
{
    node = std::shared_ptr<ValNode>(n);
}

Value::Value(const std::shared_ptr<ValNode>& n)
{
    node = n;
}

Value::Value(ast::NPtr<ast::FunctionDef>::type fd)
{
    auto n = std::make_shared<FuncNode>();
    n->fd = move(fd);
    node = n;
}

void Value::makeScalar()
{
    node = std::make_shared<NormalScalarNode>();
}

Value Value::seq()
{
    return new LevelNSeqNode();
}

Value Value::scalarSeq(size_t width)
{
    return new Level1SeqNode(width);
}

}
