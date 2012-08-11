#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"
#include "Global.h"

#include <cassert>
#include <queue>

using namespace ast;
using namespace sa;

//This is for the section of Sema 3 that deals with intrinsics

//This relies on nobody else using counter, and the intrinsics being in the right order
//and all in one file

#define ARITH(op, type) \
    case __COUNTER__: \
    Annotate(getChild(0)->Value().getScalarAs<type>() \
        op getChild(1)->Value().getScalarAs<type>()); \
    break

#define U_ARITH(op, type) \
    case __COUNTER__: \
    Annotate(op getChild(0)->Value().getScalarAs<type>()); \
    break

#define INTEGER(op) \
    ARITH(op, signed char); \
    ARITH(op, short); \
    ARITH(op, long); \
    ARITH(op, long long)

#define U_INTEGER(op) \
    U_ARITH(op, signed char); \
    U_ARITH(op, short); \
    U_ARITH(op, long); \
    U_ARITH(op, long long)

#define NUMERIC(op) \
    INTEGER(op); \
    ARITH(op, float); \
    ARITH(op, double); \
    ARITH(op, long double)

void IntrinCallExpr::inferType(Sema&)
{
    //may need to change this if we do other things here
    for (auto& n : Children())
        if (!n->Value())
            return;

    switch (intrin_id)
    {
        //numeric intrinsics

        NUMERIC(+);
        NUMERIC(-);
        NUMERIC(*);
        NUMERIC(/);
        
        NUMERIC(<);
        INTEGER(<=);
        NUMERIC(>);
        INTEGER(<=);
        INTEGER(==);
        INTEGER(!=);

        INTEGER(&);
        INTEGER(|);
        INTEGER(^);
        INTEGER(%);

        U_INTEGER(++);
        U_INTEGER(--);
    default:
        ;
    }
}