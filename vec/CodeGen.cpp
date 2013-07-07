#include "CodeGen.h"
#include "AstWalker.h"
#include "SemaNodes.h"
#include "Value.h"
#include "Global.h"
#include "Intrinsic.h"
#include "Error.h"

#include <set>

using namespace cg;
using namespace llvm;

#define IGNORED nullptr //denotes an llvm::Value* that should be ignored

//IMPORTANT: the order of evaluation of function arguments is not specified.
//DO NOT CALL GENERATE IN AN ARGUMENT LIST. BAD THINGS WILL HAPPEN.

CodeGen::CodeGen(std::string& outfile)
    : curBB(nullptr), curFunc(nullptr)
{
    if (Global().numErrors != 0) //cannot generate code if there are errors
    {
        err::Error(err::fatal, tok::Location()) << Global().numErrors << " errors have occured";
        throw err::FatalError();
    }

    curMod.reset(new Module(outfile + ".bc", getGlobalContext()));

    Global().entryPt->Value().getFunc()->gen(*this);

    std::string errors;
    llvm::raw_fd_ostream fout(outfile.c_str(), errors);

	PassManager pm;
    //pm.add(createDeadCodeEliminationPass());
    //add a few passes to make the IR easier to read
    pm.add(createCFGSimplificationPass());
    //pm.add(createGVNPass());
	pm.add(createPrintModulePass(&fout));
    pm.add(createVerifierPass(llvm::VerifierFailureAction::PrintMessageAction));
	pm.run(*curMod);
}

//---------------------------------------------------
//control flow nodes

Value* ast::Lambda::generate(CodeGen& cgen)
{
    //TODO: name mangling?

    //get or create function
    cgen.curFunc = cgen.curMod->getFunction(Global().getIdent(name));

    if (!cgen.curFunc)
    {
        cgen.curFunc = Function::Create(
            dyn_cast<llvm::FunctionType>(Type().toLLVM()),
            llvm::GlobalValue::ExternalLinkage, Global().getIdent(name), cgen.curMod.get());
    }

    getChildA()->gen(cgen);

    return cgen.curFunc;
}

Value* ast::StmtPair::generate(CodeGen& cgen)
{
    getChildA()->gen(cgen);
    return getChildB()->gen(cgen);
}

Value* ast::ReturnStmt::generate(CodeGen& cgen)
{
    llvm::Value* retVal = getChildA()->gen(cgen);
    ReturnInst::Create(getGlobalContext(), retVal, cgen.curBB);
    //return the value so a function with a return at the end doesn't cause an error
    return retVal;
}

Value* ast::RhoStmt::generate(CodeGen& cgen)
{
    return getChild(0)->gen(cgen);
}

//a side effect of this is leaving out unreachable blocks
Value* ast::BranchStmt::generate(CodeGen& cgen)
{
    //save off the current basic block in case we're being called recursively
    BasicBlock* savedBB = cgen.curBB;

    llvmVal = cgen.curBB = BasicBlock::Create(getGlobalContext(), "", cgen.curFunc);
    llvm::Value* result = getChildA()->gen(cgen);

    BasicBlock* ift = nullptr, *iff = nullptr;
    if (ifTrue)
    {
        ift = dyn_cast<BasicBlock>(ifTrue->gen(cgen));
    
        if (ifFalse != ifTrue) //conditional
        {
            iff = dyn_cast<BasicBlock>(ifFalse->gen(cgen));
            BranchInst::Create(ift, iff, result, cgen.curBB);
        }
        else
            BranchInst::Create(ift, cgen.curBB);
    }
    //else we have a return stmt that generates its own terminator

    //restore old BB and return ours
    cgen.curBB = savedBB;
    return llvmVal;
}

Value* ast::PhiExpr::generate(CodeGen& cgen)
{
    PHINode* ret = PHINode::Create(Type().toLLVM(), inputs.size(), "", cgen.curBB);

    for (auto pred : inputs)
    {
        BasicBlock* block = dyn_cast<BasicBlock>(pred->gen(cgen));
        //kind of hackish. The value for a branch is the value of its child
        ret->addIncoming(pred->getChildA()->gen(cgen), block);
    }

    return ret;
}

Value* ast::TmpExpr::generate(CodeGen& cgen)
{
    return setBy->gen(cgen);
}

//---------------------------------------------------
//expression nodes

Value* ast::NullExpr::generate(CodeGen&)
{
    return IGNORED;
}

Value* ast::NullStmt::generate(CodeGen&)
{
    return IGNORED;
}

Value* ast::DeclExpr::generate(CodeGen& cgen)
{
    llvm::Value* addr = new AllocaInst(Type().toLLVM(), Global().getIdent(name), cgen.curBB);
    Annotate(addr);
    //TODO: call constructor

    //FIXME: this probably will make lots of extra loads. separate decl and var exprs in sema
    //is this even useful ever?
    return new LoadInst(addr, "", cgen.curBB);
}

Value* ast::VarExpr::generate(CodeGen& cgen)
{
    return new LoadInst(Address(), "", cgen.curBB);
}

Value* ast::ConstExpr::generate(CodeGen&)
{
    llvm::Value* ret;
    if (Type().getPrimitive().isInt())
        ret = ConstantInt::getSigned(Type().toLLVM(), Value().getScalarAs<int64_t>());

    else if (Type().getPrimitive().isFloat())
        ret = ConstantFP::get(Type().toLLVM(), (double)Value().getScalarAs<long double>());

    else if (Type() == typ::boolean)
        ret = ConstantInt::get(Type().toLLVM(), Value().getScalarAs<bool>());

    else
    {
        assert("this type of constant unimplemented");
        ret = UndefValue::get(Type().toLLVM()); //FIXME!!!
    }

    return ret;
}

Value* ast::AssignExpr::generate(CodeGen& cgen)
{
    //TODO: it's stupid I have to prefix Value with llvm in these functions
    llvm::Value* val = getChildB()->gen(cgen);
    getChildA()->gen(cgen);
    llvm::Value* addr = getChildA()->Address();
    Annotate(addr); //because assignments are lvalues

    new StoreInst(val, addr, cgen.curBB);
    //TODO: copy constructor

    return val;
}

Value* ast::ArithCast::generate(CodeGen& cgen)
{
    llvm::Value* arg = getChildA()->gen(cgen);

    return CastInst::Create(CastInst::getCastOpcode(arg, true, Type().toLLVM(), true),
        arg, Type().toLLVM(), "", cgen.curBB);
}

//---------------------------------------------------
//intrinsic node

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
