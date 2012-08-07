#ifndef VALUE_H
#define VALUE_H

#include "Type.h"

#include <memory>
#include <array>

//this is implemented similarly to types, except that is likely that we won't need values
//for long after they're created. So, a reference counting implementation makes the most
//sense here.

namespace ast
{
    struct FunctionDef;
}

namespace val
{
    struct ValNode;
    typedef std::array<char, 8> byteVec;

    class Value
    {
        friend struct ScalarRefNode; //needs to access node
        friend struct Level1SeqNode;
        friend struct LevelNSeqNode;
        friend Value fromNode(ValNode* n);
        friend Value makeScalar();
        friend Value seq(typ::Type t);
        friend Value scalarSeq(typ::Type t);

        std::shared_ptr<ValNode> node; //I guess this is spimpl

        byteVec& getBytes();

    public:
        Value() {}

        typ::Type& Type();

        //create a copy that can be modifed independently
        Value Duplicate();

        //LLVMValue getLLVMValue();

        Value& getRefTarget();
        void setRefTarget(Value& v);

        Value getSeqElem(size_t pos); //can be modifed, changing the original sequence
        void appendSeqElem(const Value& v);

        template<typename T>
        T& getScalarAs()
        {
            return *reinterpret_cast<T*>(getBytes().data());
        }

        ast::FunctionDef*& getFunc();
    };

    //factory functions

    Value makeScalar(); //needed because fromBytesOf is a template

    template<typename T>
    Value fromBytesOf(typ::Type t, T val)
    {
        Value ret = makeScalar();
        ret.getScalarAs<T>() = val;
        ret.Type() = t;
        return ret;
    }

    Value seq(typ::Type t);
    Value scalarSeq(typ::Type t);
}
        

#endif