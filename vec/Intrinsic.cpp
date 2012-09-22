#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"
#include "Global.h"
#include "Value.h"
#include "Intrinsic.h"

#include <cassert>
#include <queue>

using namespace ast;
using namespace sa;
using namespace intr;

//This is for the section of Sema 3 that deals with intrinsics

//This relies on the intrinsics being in the right order and all in one file

#define ARITH(op, type, opid, typid) \
    case (opid) + (typid): \
        Annotate(getChild(0)->Value().getScalarAs<type>() \
            op getChild(1)->Value().getScalarAs<type>()); \
    break

#define U_ARITH(op, type, opid, typid) \
    case (opid) + (typid): \
        Annotate(op getChild(0)->Value().getScalarAs<type>()); \
    break

#define INTEGER(op, opid) \
    ARITH(op, signed char, opid, TYPES::INT8); \
    ARITH(op, short,       opid, TYPES::INT16); \
    ARITH(op, long,        opid, TYPES::INT32); \
    ARITH(op, long long,   opid, TYPES::INT64)

#define U_INTEGER(op, opid) \
    U_ARITH(op, signed char, opid, TYPES::INT8); \
    U_ARITH(op, short,       opid, TYPES::INT16); \
    U_ARITH(op, long,        opid, TYPES::INT32); \
    U_ARITH(op, long long,   opid, TYPES::INT64)

#define NUMERIC(op, opid) \
    INTEGER(op, opid); \
    ARITH(op, float,       opid, TYPES::FLOAT); \
    ARITH(op, double,      opid, TYPES::DOUBLE); \
    ARITH(op, long double, opid, TYPES::LONG_DOUBLE)

void IntrinCallExpr::preExec(Exec&)
{
    //may need to change this if we do other things here
    for (auto& n : Children())
        if (!n->Value())
            return;

    switch (intrin_id)
    {
        //numeric intrinsics

        NUMERIC(+, OPS::PLUS);
        NUMERIC(-, OPS::MINUS);
        NUMERIC(*, OPS::TIMES);
        NUMERIC(/, OPS::DIVIDE);
        
        NUMERIC(<, OPS::LESS);
        INTEGER(<=, OPS::NOGREATER);
        NUMERIC(>, OPS::GREATER);
        INTEGER(<=, OPS::NOLESS);
        INTEGER(==, OPS::EQUAL);
        INTEGER(!=, OPS::NOTEQUAL);

        INTEGER(&, OPS::BITAND);
        INTEGER(|, OPS::BITOR);
        INTEGER(^, OPS::BITXOR);
        INTEGER(%, OPS::MOD);

        U_INTEGER(++, OPS::INCREMENT);
        U_INTEGER(--, OPS::DECREMENT);

        ARITH(&&, bool, OPS::AND, 0);
        ARITH(||, bool, OPS::OR, 0);
        U_ARITH(!, bool, OPS::NOT, 0);
    default:
        ;
    }
}