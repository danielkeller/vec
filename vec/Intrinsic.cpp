#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"
#include "Global.h"
#include "Value.h"
#include "Intrinsic.h"
#include "CodeGen.h"

#include <cassert>
#include <queue>

//preExec

namespace ast {
using namespace sa;
using namespace intr;

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

        U_ARITH(!, bool, OPS::NOT, 0);
    default:
        ;
    }
}
} //end using namespace


//code generation

namespace ast {
using namespace cg;
using namespace llvm;

#define INTS(op) \
    case (op) + (intr::TYPES::INT8):  \
    case (op) + (intr::TYPES::INT16): \
    case (op) + (intr::TYPES::INT32): \
    case (op) + (intr::TYPES::INT64)

#define FLOATS(op) \
    case (op) + (intr::TYPES::FLOAT): \
    case (op) + (intr::TYPES::DOUBLE): \
    case (op) + (intr::TYPES::LONG_DOUBLE)

#define NUMERICS(op) \
    INTS(op): \
    FLOATS(op)
    
#define SET_FOR(var, types, op, llvmop) \
        types(op): \
            var = llvmop; \
            break

#define SET_BIN_OP(types, op, llvmop)   SET_FOR(binOp, types, op, llvmop)
#define SET_PRED(types, op, llvmop)     SET_FOR(pred,  types, op, llvmop)

Value* ast::IntrinCallExpr::generate(CodeGen& cgen)
{
    //first handle the easy cases
    Instruction::BinaryOps otherOp = Instruction::BinaryOps(-1);
    Instruction::BinaryOps binOp;

    switch (intrin_id)
    {
        SET_BIN_OP(INTS, intr::OPS::PLUS, Instruction::BinaryOps::Add);
        SET_BIN_OP(FLOATS, intr::OPS::PLUS, Instruction::BinaryOps::FAdd);
        SET_BIN_OP(INTS, intr::OPS::MINUS, Instruction::BinaryOps::Sub);
        SET_BIN_OP(FLOATS, intr::OPS::MINUS, Instruction::BinaryOps::FSub);
        SET_BIN_OP(INTS, intr::OPS::TIMES, Instruction::BinaryOps::Mul);
        SET_BIN_OP(FLOATS, intr::OPS::TIMES, Instruction::BinaryOps::FMul);
        SET_BIN_OP(INTS, intr::OPS::DIVIDE, Instruction::BinaryOps::SDiv);
        SET_BIN_OP(FLOATS, intr::OPS::DIVIDE, Instruction::BinaryOps::FDiv);

        SET_BIN_OP(INTS, intr::OPS::BITAND, Instruction::BinaryOps::And);
        SET_BIN_OP(INTS, intr::OPS::BITOR, Instruction::BinaryOps::Or);
        SET_BIN_OP(INTS, intr::OPS::BITXOR, Instruction::BinaryOps::Xor);

    default:
        binOp = otherOp;
    }

    if (binOp != otherOp)
    {
        llvm::Value* lhs = getChild(0)->gen(cgen);
        llvm::Value* rhs = getChild(1)->gen(cgen);
        return BinaryOperator::Create(binOp, lhs, rhs, "", cgen.curBB);
    }


    //now the comparison operators
    CmpInst::Predicate otherPr = CmpInst::Predicate(-1);
    CmpInst::Predicate pred;

    switch (intrin_id)
    {
        SET_PRED(INTS, intr::OPS::LESS, CmpInst::Predicate::ICMP_SLT);
        SET_PRED(FLOATS, intr::OPS::LESS, CmpInst::Predicate::FCMP_OLT);
        SET_PRED(INTS, intr::OPS::GREATER, CmpInst::Predicate::ICMP_SGT);
        SET_PRED(FLOATS, intr::OPS::GREATER, CmpInst::Predicate::FCMP_OGT);

        SET_PRED(INTS, intr::OPS::NOGREATER, CmpInst::Predicate::ICMP_SLE);
        SET_PRED(INTS, intr::OPS::NOLESS, CmpInst::Predicate::ICMP_SGE);
        SET_PRED(INTS, intr::OPS::EQUAL, CmpInst::Predicate::ICMP_EQ);
        SET_PRED(INTS, intr::OPS::NOTEQUAL, CmpInst::Predicate::ICMP_NE);

    default:
        pred = otherPr;
    }

    if (pred != otherPr)
    {
        llvm::Value* lhs = getChild(0)->gen(cgen);
        llvm::Value* rhs = getChild(1)->gen(cgen);
        return CmpInst::Create(
            getChild(0)->Type().getPrimitive().isInt()
                ? Instruction::OtherOps::ICmp
                : Instruction::OtherOps::FCmp,
            (unsigned short)pred, lhs, rhs, "", cgen.curBB);
    }
    assert(false && "not implemented");
    return nullptr;
}
}