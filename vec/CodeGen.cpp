#include "CodeGen.h"
#include "AstWalker.h"
#include "SemaNodes.h"
#include "Value.h"
#include "Global.h"
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
    llvm::Value* addr = new AllocaInst(Type().toLLVM(), Global().getIdent(Name()), cgen.curBB);
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