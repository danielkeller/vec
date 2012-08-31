#include "CodeGen.h"
#include "AstWalker.h"
#include "SemaNodes.h"
#include "Value.h"
#include "Global.h"
#include "Intrinsic.h"

#include <set>

using namespace cg;
using namespace llvm;

CodeGen::CodeGen(std::string& outfile)
    : curBB(nullptr), curFunc(nullptr)
{
    curMod = new Module(outfile + ".bc", getGlobalContext());

    //gather up all functions
    /*
    std::set<ast::FunctionDef*> funcs;
    for (auto m : Global().allModules)
        for (auto fd : sa::Subtree<ast::FuncDeclExpr>(m))
            if (fd->Value())
                funcs.insert(fd->Value().getFunc().get());

        */
    for (auto m : Global().allModules)
        for (auto fd : sa::Subtree<ast::FuncDeclExpr>(m))
            fd->generate(*this);

    std::string errors;
    llvm::raw_fd_ostream fout(outfile.c_str(), errors);

	PassManager pm;
    pm.add(createVerifierPass(llvm::VerifierFailureAction::PrintMessageAction));
    pm.add(createDeadCodeEliminationPass());
	pm.add(createPrintModulePass(&fout));
	pm.run(*curMod);
}

//---------------------------------------------------
//control flow nodes

Value* ast::FuncDeclExpr::generate(CodeGen& gen)
{
    //get or create function
    gen.curFunc = gen.curMod->getFunction(Global().getIdent(name));

    if (!gen.curFunc)
    {
        gen.curFunc = Function::Create(
            dyn_cast<llvm::FunctionType>(Type().toLLVM()),
            llvm::GlobalValue::ExternalLinkage, Global().getIdent(name), gen.curMod);
    }

    ast::FunctionDef* def = Value().getFunc().get();
    if (!def || gen.curFunc->size())
        return gen.curFunc; //external or already defined

    gen.curBB = BasicBlock::Create(getGlobalContext(), "FuncEntry", gen.curFunc);

    def->generate(gen);

    return gen.curFunc;
}

Value* ast::FunctionDef::generate(CodeGen& gen)
{
    return getChildA()->generate(gen);
}

Value* ast::StmtPair::generate(CodeGen& gen)
{
    getChildA()->generate(gen);
    return getChildB()->generate(gen);
}

Value* ast::ReturnStmt::generate(CodeGen& gen)
{
    llvm::Value* retVal = getChildA()->generate(gen);
    return ReturnInst::Create(getGlobalContext(), retVal, gen.curBB);
}

//---------------------------------------------------
//expression nodes

Value* ast::DeclExpr::generate(CodeGen& gen)
{
    llvm::Value* addr = new AllocaInst(Type().toLLVM(), Global().getIdent(name), gen.curBB);
    Annotate(addr);
    //TODO: call constructor

    //FIXME: this probably will make lots of extra loads. separate decl and var exprs in sema
    //is this even useful ever?
    return new LoadInst(addr, "", gen.curBB);
}

Value* ast::VarExpr::generate(CodeGen& gen)
{
    return new LoadInst(Address(), "", gen.curBB);
}

Value* ast::ConstExpr::generate(CodeGen&)
{
    llvm::Value* ret;
    if (Type().getPrimitive().isInt())
        ret = ConstantInt::getSigned(Type().toLLVM(), Value().getScalarAs<int64_t>());

    else if (Type().getPrimitive().isFloat())
        ret = ConstantFP::get(Type().toLLVM(), (double)Value().getScalarAs<long double>());

    else
        ret = UndefValue::get(Type().toLLVM()); //FIXME!!!

    return ret;
}

Value* ast::AssignExpr::generate(CodeGen& gen)
{
    //TODO: it's stupid I have to prefix Value with llvm in these functions
    llvm::Value* val = getChildB()->generate(gen);
    getChildA()->generate(gen);
    llvm::Value* addr = getChildA()->Address();
    Annotate(addr); //because assignments are lvalues

    new StoreInst(val, addr, gen.curBB);
    //TODO: copy constructor

    return val;
}

Value* ast::ArithCast::generate(CodeGen& gen)
{
    llvm::Value* arg = getChildA()->generate(gen);

    return CastInst::Create(CastInst::getCastOpcode(arg, true, Type().toLLVM(), true),
        arg, Type().toLLVM(), "", gen.curBB);
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

Value* ast::IntrinCallExpr::generate(CodeGen& gen)
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
        return BinaryOperator::Create(binOp,
            getChild(0)->generate(gen), getChild(1)->generate(gen),
            "", gen.curBB);
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
        return CmpInst::Create(
            getChild(0)->Type().getPrimitive().isInt()
                ? Instruction::OtherOps::ICmp
                : Instruction::OtherOps::FCmp,
            (unsigned short)pred,
            getChild(0)->generate(gen), getChild(1)->generate(gen),
            "", gen.curBB);
    }

    //now the short-circuiting operators
    if (intrin_id == intr::OPS::AND || intrin_id == intr::OPS::OR)
    {
        /*
        before (a) ---> alt (b)
                \        \
                 \------>after (r = phi before:a, after:b)

        r = a && b 
            if (a)
                r = b
            else
                r = a

        r = a || b
            if (!a)
                r = b
            else
                r = a
        */
        BasicBlock* before = gen.curBB;
        BasicBlock* alt = BasicBlock::Create(getGlobalContext(), "sc-else", gen.curFunc);
        BasicBlock* after = BasicBlock::Create(getGlobalContext(), "sc-after", gen.curFunc);

        llvm::Value* lhs = getChild(0)->generate(gen);

        if (intrin_id == intr::OPS::AND)
            BranchInst::Create(alt, after, lhs, before); //if !a
        else
            BranchInst::Create(after, alt, lhs, before);

        gen.curBB = alt; //set the current basic block to alt so the rhs goes there
        llvm::Value* rhs = getChild(1)->generate(gen);

        BranchInst::Create(after, alt);
        gen.curBB = after;

        PHINode* phi = PHINode::Create(Type().toLLVM(), 2, "sc-phi", after);
        phi->addIncoming(lhs, before);
        phi->addIncoming(rhs, alt);

        return phi;
    }

    assert(false && "not implemented");
    return nullptr;
}