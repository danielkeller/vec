#include "Exec.h"
#include "Error.h"
#include "Global.h"
#include "Value.h"
#include "LLVM.h"
#include "AstWalker.h"

#include <cassert>
#include <queue>

using namespace ast;
using namespace sa;

//Compile-time code execution:

//This is type inference, overload resolution, and template instantiation
//any constants are calcuated, and any constant, non-side-effecting
//expressions are removed. Any remaining code marked "static" is executed

//convenience for overload resolution
typedef std::pair<typ::TypeCompareResult, DeclExpr*> ovr_result;
struct pairComp
{
    bool operator()(const ovr_result& lhs, const ovr_result& rhs) {return rhs.first < lhs.first;}
};
typedef std::priority_queue<ovr_result, std::vector<ovr_result>, pairComp> ovr_queue;

void OverloadableExpr::resolveOverload(typ::Type argType, Exec* ex)
{
    ovr_queue result;

    std::vector<DeclExpr*> functions = sco->getVarDefs(name);

    for (auto func : functions)
    {
        //ignore non-functions
        if (!func->Type().getFunc().isValid())
            continue;

        result.push(
            std::make_pair(
                argType.compare(func->Type().getFunc().arg()),
                func)
            );
    }

    //TODO: find a better way?
    Node0* call = dynamic_cast<Node0*>(this);

    //TODO: now try template functions
    if (result.size() == 0)
    {
        err::Error(call->loc) << "function '" << name << "' is not defined in this scope"
            << err::underline << call->loc << err::caret;
        call->Annotate(typ::error);
        return;
    }

    ovr_result firstChoice = result.top();

    if (!firstChoice.first.isValid())
    {
        err::Error(call->loc) << "no accessible instance of overloaded function '" << name
            << "' matches arguments of type " << argType << err::underline << call->loc << err::caret;
        call->Annotate(typ::error);
    }
    else if (result.size() > 1 && (result.pop(), firstChoice.first == result.top().first))
    {
        err::Error ambigErr(call->loc);
        ambigErr << "overloaded call to '" << name << "' is ambiguous" << err::underline
            << call->loc << err::caret << firstChoice.second->loc << err::note << "could be" << err::underline;

        while (result.size() && firstChoice.first == result.top().first)
        {
            ambigErr << result.top().second->loc << err::note << "or" << err::underline;
            result.pop();
        }
        ovrResult = firstChoice.second; //recover
        call->Annotate(ovrResult->Type().getFunc().ret());
    }
    else //success
    {
        ovrResult = firstChoice.second;
        call->Annotate(ovrResult->Type().getFunc().ret());

        //if its an intrinsic, switch it to a special node
        if (exact_cast<IntrinDeclExpr*>(ovrResult))
        {
            Node0* parent = call->parent;

            auto iCall = makeICall();

            parent->replaceDetachedChild(Ptr(iCall));

            if (ex)
                iCall->preExec(*ex);
        }

        //TODO: process the function
    }
}

void AssignExpr::preExec(Exec& ex)
{
    getChildA()->preExec(ex);
    getChildB()->preExec(ex);
    //try to merge types if its a decl
    if (Type().compare(getChildB()->Type()) == typ::TypeCompareResult::invalid)
    {
        //first check if we can do arith conversion
        //FIXME: this should go in copy constructor code
        if (Type().getPrimitive().isArith() && getChildB()->Type().getPrimitive().isArith())
        {
            setChildB(Ptr(new ArithCast(Type(), detachChildB())));
            getChildB()->preExec(ex);
        }
        else
        {
            err::Error(getChildA()->loc) << "cannot convert from "
                << getChildB()->Type() << " to "
                << getChildA()->Type() << " in assignment"
                << err::underline << opLoc << err::caret
                << getChildB()->loc << err::underline;
            return;
        }
    }

    if (getChildB()->Value())
    {
        Annotate(getChildB()->Value());
    }
}

void OverloadCallExpr::preExec(Exec& ex)
{
    typ::TupleBuilder builder;
    for (auto& arg : Children())
    {
        arg->preExec(ex);
        builder.push_back(arg->Type(), Global().reserved.null);
    }
    typ::Type argType = typ::mgr.makeTuple(builder);

    resolveOverload(argType, &ex);
}

void BinExpr::preExec(Exec& ex)
{
    getChildA()->preExec(ex);
    getChildB()->preExec(ex);

    //TODO: should this be an intrinsic?
    if (op == tok::colon) //function pointer call
    {
        Node0* lhs = getChildA();
        typ::FuncType ft = lhs->Type().getFunc();
        if (!ft.isValid())
        {
            err::Error(lhs->loc) << "cannot call object of non-function type "
                << lhs->Type() << err::underline
                << opLoc << err::caret;
            Type() = lhs->Type(); //sorta recover
        }
        else
        {
            Annotate(ft.ret());
            if (!ft.arg().compare(getChildB()->Type()).isValid())
                err::Error(getChildA()->loc)
                    << "function arguments are inappropriate for function"
                    << ft.arg().to_str() << " != " << getChildB()->Type()
                    << err::underline << opLoc << err::caret
                    << getChildB()->loc << err::underline;
        }
    }
    else if (tok::CanBeOverloaded(op))
    {
        typ::Type lhs_t = getChildA()->Type();
        typ::Type rhs_t = getChildB()->Type();
        //first do usual arithmetic conversions, if applicable
        //these are stated in the c standard §6.3.1.8 (omitting unsigned types)
        //if unsigned types are added, they should conform to the standard

        //FIXME: this doesn't apply to all operators, ie (,)
        //TODO: integer-only cases (==, %, etc)
        if (lhs_t.getPrimitive().isArith() && rhs_t.getPrimitive().isArith() && lhs_t != rhs_t)
        {
            Ptr childA = detachChildA();
            Ptr childB = detachChildB();

            auto promoteOther = [&](typ::Type target) -> bool
            {
                if (lhs_t == target)
                {
                    childB = Ptr(new ArithCast(target, move(childB)));
                    childB->preExec(ex);
                }
                else if (rhs_t == target)
                {
                    childA = Ptr(new ArithCast(target, move(childA)));
                    childA->preExec(ex);
                }
                else
                    return false;
                return true; //if it did something
            };

            //this uses short-circuiting to avoid writing ugly nested/empty ifs

            //convert to the largest floating point type of either operand
            promoteOther(typ::float80)
            || promoteOther(typ::float64)
            || promoteOther(typ::float32)
            //If the above three conditions are not met (none of the operands are of
            //floating types), then integral conversions are performed on the operands as follows:
            //convert to the largest integer type of either operand
            || promoteOther(typ::int64)
            || promoteOther(typ::int32)
            || promoteOther(typ::int16); //deviate from the standard for consistency

            setChildA(move(childA));
            setChildB(move(childB));
        }

        typ::TupleBuilder builder;
        builder.push_back(getChildA()->Type(), Global().reserved.null);
        builder.push_back(getChildB()->Type(), Global().reserved.null);
        typ::Type argType = typ::mgr.makeTuple(builder);

        resolveOverload(argType, &ex);
    }
    else
        assert("operator not yet implemented");
}

#define LL_BITS (sizeof(long long)*8)

namespace
{
    val::Value signExt(long long val, size_t fromWidth, size_t toWidth)
    {
        //shrinking
        if (toWidth < fromWidth)
        {
            //clear top toWidth bits
            val = (val << (LL_BITS - toWidth)) >> (LL_BITS - toWidth);
        }
        else if (val & (1ll << (fromWidth - 1))) //sign-extending
        {
            while (fromWidth < toWidth)
            {
                val |= 0xFF << fromWidth;
                fromWidth += 8;
            }
        }
        //else zero extending
        return val::Value(val);
    }

    val::Value fromLD(long double from, typ::Type to)
    {
        if (to == typ::float32)
            return val::Value((float)from);
        else if (to == typ::float64)
            return val::Value((double)from);
        else
            return val::Value(from);
    }

    long double toLD(const val::Value& from, typ::Type type)
    {
        if (type == typ::float32)
            return from.getScalarAs<float>();
        else if (type == typ::float64)
            return from.getScalarAs<double>();
        else
            return from.getScalarAs<long double>();
    }
}

void ArithCast::preExec(Exec&)
{
    //Assume child has already been executed
    if (!getChildA()->Value())
        return;

    auto from = getChildA()->Type().getPrimitive();
    auto to = Type().getPrimitive();

    if (from.isInt()) 
    {
        int fromWidth = from.toLLVM()->getIntegerBitWidth();
        long long val = getChildA()->Value().getScalarAs<long long>();

        if (to.isInt()) // int -> int
        {
            int toWidth = to.toLLVM()->getIntegerBitWidth();
            Annotate(signExt(val, fromWidth, toWidth));
        }
        else //int -> float
        {
            long long temp1 = signExt(val, fromWidth, LL_BITS);
            Annotate(fromLD((long double)temp1, to));
        }
    }
    else
    {
        long double temp2 = toLD(getChildA()->Value(), from);

        if (to.isInt()) // float -> int
        {
            Annotate(signExt((long long)temp2, LL_BITS, to.toLLVM()->getIntegerBitWidth()));
        }
        else //float -> float
        {
            Annotate(fromLD(temp2, to));
        }
    }
}

void ListifyExpr::preExec(Exec& ex)
{
    typ::Type conts_t = getChild(0)->Type();
    Annotate(typ::mgr.makeList(conts_t, Children().size()));
    for (auto& c : Children())
    {
        c->preExec(ex);
        if (conts_t.compare(c->Type()) == typ::TypeCompareResult::invalid)
            err::Error(getChild(0)->loc) << "list contents must be all the same type, " << conts_t
                << " != " << c->Type() << err::underline << c->loc << err::underline;
    }
}

void TuplifyExpr::preExec(Exec& ex)
{
    typ::TupleBuilder builder;
    for (auto& c : Children())
    {
        c->preExec(ex);
        builder.push_back(c->Type(), Global().reserved.null);
    }
    Annotate(typ::mgr.makeTuple(builder));
}

void StmtPair::preExec(Exec& ex)
{
    getChildA()->preExec(ex);
    getChildB()->preExec(ex);
}

void ReturnStmt::preExec(Exec& ex)
{
    getChildA()->preExec(ex);
    //first check if we can do arith conversion
    //FIXME: this should go in copy constructor code
    //TODO: this
 /*
    if (Type().getPrimitive().isArith() && getChildA()->Type().getPrimitive().isArith())
        setChildA(Ptr(new ArithCast(Type(), detachChildA())));
    else
        err::Error(getChildA()->loc) << "cannot convert from "
            << getChildB()->Type() << " to "
            << getChildA()->Type() << " in assignment"
            << err::underline << opLoc << err::caret
            << getChildB()->loc << err::underline;
            */
    Annotate(getChildA()->Type());
}

void RhoStmt::preExec(Exec& ex)
{
    for (auto& c : Children())
        c->preExec(ex);
}

void BranchStmt::preExec(Exec& ex)
{
    getChildA()->preExec(ex);

    if (ifTrue != ifFalse //conditional, so child is condition
        && getChildA()->Type().compare(typ::boolean) == typ::TypeCompareResult::invalid)
        err::Error(getChildA()->loc) << "expected boolean type, got " << getChildA()->Type()
            << err::underline;
}

void PhiExpr::preExec(Exec&)
{
    //TODO: to do this at compile time, Exec will have to track the predecessor block if one
    //exists

    assert(inputs.size() && "empty phi");
    Annotate(inputs[0]->Type());
    for (auto pred : inputs)
        if (!pred->Type().compare(Type()).isValid())
            err::Error(loc) << "type mismatch, " << Type() << " vs " << pred->Type()
                << err::underline;
}

//TOOD: return type and value/call?
void Exec::processFunc (ast::DeclExpr* n)
{
    //TODO: merge and use Lambda's type
    typ::FuncType ft = n->Type().getFunc();
    assert(ft.isValid() && "that's not a function!");

    if (!n->Value())
        err::Error(n->loc) << "function '" << n->name << "' is never defined";
    else
    {
        Lambda* def = n->Value().getFunc().get();

        typ::Type arg = ft.arg();
        typ::TupleType args = arg.getTuple();

        if (args.isValid())
        {
            if (def->params.size() != args.size())
            {
                err::Error(n->loc) << "incorrect number of parameters for function, expected "
                    << utl::to_str(args.size()) << ", got " << def->params.size();
                return;
            }

            for (size_t i = 0; i < args.size(); ++i)
            {
                def->sco->getVarDef(def->params[i])->Annotate(args.elem(i));
            }
        }
        else
            def->sco->getVarDef(def->params[0])->Annotate(arg);

        def->getChildA()->preExec(*this);

        for (auto ret : Subtree<ReturnStmt>(def))
            if (ft.ret().compare(ret->Type()) == typ::TypeCompareResult::invalid)
                err::Error(ret->loc) << "cannot convert from "
                    << ret->getChildA()->Type() << " to " << ft.ret()
                    << " in function return" << err::underline;
    }
}

void Exec::processMod(ast::Module* mod)
{
    //TODO: warn about cyclic imports
    mod->execStarted = true;
    for (auto import : mod->imports)
        if (!import->execStarted)
            processMod(import);

    //change functions into values so we don't enter them early
    for (auto func : Subtree<Lambda>(mod).cached())
    {
        Node0* parent = func->parent;
        ConstExpr* funcVal = new ConstExpr(func->loc);
        typ::Type funcType = func->Type();

        funcVal->Annotate(funcType, val::Value(func->detachSelfAs<Lambda>()));
        parent->replaceDetachedChild(Ptr(funcVal));
    }

    mod->getChildA()->preExec(*this);
}

Exec::Exec(ast::Module* mainMod)
{
    typ::mgr.makeLLVMTypes();

    processMod(mainMod);

    //now find the entry point and go!!

    DeclExpr* mainVar = mainMod->priv.getVarDef(Global().reserved.main);
    if (!mainVar)
    {
        err::Error(err::fatal, mainMod->loc) << "variable 'main' undefined";
        throw err::FatalError();
    }

    Ptr args = MkNPtr(new ConstExpr(mainVar->loc));
    args->Annotate(typ::mgr.makeList(Global().reserved.string_t));

    OverloadCallExpr entryPt(
        MkNPtr(new VarExpr(mainVar->name, mainVar->sco, mainVar->loc)),
        move(args), mainVar->loc);

    //resolveOverload may be modified at some point to call processFunc
    entryPt.resolveOverload(typ::mgr.makeList(Global().reserved.string_t), this);

    if (!entryPt.ovrResult)
    {
        err::Error(err::fatal, mainVar->loc) << "appropriate 'main' function not found" << err::underline;
        throw err::FatalError();
    }

    Global().entryPt = entryPt.ovrResult;
    processFunc(entryPt.ovrResult);
}

IntrinCallExpr* OverloadCallExpr::makeICall()
{
    IntrinDeclExpr* intrin = assert_cast<IntrinDeclExpr*>(ovrResult, "this is not an intrin");
    return new IntrinCallExpr(detachSelfAs<OverloadCallExpr>(), intrin->intrin_id);
}

IntrinCallExpr* BinExpr::makeICall()
{
    IntrinDeclExpr* intrin = assert_cast<IntrinDeclExpr*>(ovrResult, "this is not an intrin");
    return new IntrinCallExpr(detachSelfAs<BinExpr>(), intrin->intrin_id);
}
