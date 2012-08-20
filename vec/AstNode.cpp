#include "SemaNodes.h"
#include "Value.h"

namespace ast
{

//various extra info about a node that may or may not be there
class Annotation
{
    friend class Node0;
    typ::Type type;
    val::Value value;
    bool lval;

    //add more as neccesary
    void set (typ::Type t)
    {
        type = t;
    }

    void set (const val::Value& v)
    {
        value = v;
    }

    void set (typ::Type t, const val::Value& v)
    {
        type = t;
        value = v;
    }
};

void Node0::makeAnnot()
{
    if (!Annot())
        Annot().reset(new Annotation());
}

void Node0::Annotate(const val::Value& v)
{
    makeAnnot();
    Annot()->set(v);
}

void Node0::Annotate(const typ::Type t)
{
    makeAnnot();
    Annot()->set(t);
}

void Node0::Annotate(typ::Type t, const val::Value& v)
{
    makeAnnot();
    Annot()->set(t, v);
}

typ::Type Node0::Type()
{
    return Annot() ? Annot()->type : typ::error;
}

val::Value& Node0::Value()
{
    return Annot() ? Annot()->value : Value();
}

void Node0::AnnotDeleter::operator()(Annotation* a)
{
    delete a;
}

//print function value if it exists
//needs to be here because it uses Value
void ConstExpr::emitDot(std::ostream& os)
{
    Node0::emitDot(os);
    if (Type().getFunc().isValid())
    {
        Value().getFunc()->emitDot(os);
        os << 'n' << static_cast<Node0*>(this) << " -> n"
            << static_cast<Node0*>(Value().getFunc().get())
            << " [style=dotted, dir=back];\n";
    }
}

}