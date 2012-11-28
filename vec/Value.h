#ifndef VALUE_H
#define VALUE_H

#include "Type.h"
#include "AstNode.h"

#include <memory>
#include <array>
#include <type_traits>

//this is implemented similarly to types, except that is likely that we won't need values
//for long after they're created. So, a reference counting implementation makes the most
//sense here.

namespace ast
{
    struct Lambda;
}

namespace val
{
    struct ValNode;

    class Value
    {
        //FIXME: this isn't good
        friend struct ScalarRefNode; //needs to access node
        friend struct Level1SeqNode;
        friend struct LevelNSeqNode;

        std::shared_ptr<ValNode> node; //I guess this is spimpl

        //must be char to avoid undefined aliasing behavior
        char* getBytes() const;

        void makeScalar(); //needed because fromBytesOf is a template

        Value(const std::shared_ptr<ValNode>& n); //for internal use

    public:
        Value() {}

        Value(ValNode* n); //for internal use

        template<typename T> //causes SF on non-arithmetic types
        Value(T val, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0)
        {
            makeScalar();
            getScalarAs<T>() = val;
        }

        Value(ast::NPtr<ast::Lambda>::type fd);

        static Value seq();
        static Value scalarSeq(size_t width);

        //create a copy that can be modifed independently
        Value Duplicate();

        bool isSet() const {return bool(node);}
        operator bool() const {return isSet();}

        //LLVMValue getLLVMValue();

        Value& getRefTarget();
        void setRefTarget(Value& v);

        Value getSeqElem(size_t pos); //can be modifed, changing the original sequence
        void appendSeqElem(const Value& v);

        template<typename T>
        T& getScalarAs() const
        {
            return *reinterpret_cast<T*>(getBytes());
        }

        ast::NPtr<ast::Lambda>::type& getFunc();
    };
}
        

#endif
