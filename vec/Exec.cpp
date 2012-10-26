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
//expressions are removed. Any remaining code marked "static" is JITted
//and executed

//convenience for overload resolution
typedef std::pair<typ::TypeCompareResult, FuncDeclExpr*> ovr_result;
struct pairComp
{
    bool operator()(const ovr_result& lhs, const ovr_result& rhs) {return rhs.first < lhs.first;}
};
typedef std::priority_queue<ovr_result, std::vector<ovr_result>, pairComp> ovr_queue;

template<class T>
void Exec::resolveOverload(OverloadGroupDeclExpr* oGroup, T* call, typ::Type argType)
{
    ovr_queue result;

    for (auto func : oGroup->functions)
    {
        //funcScope is the function's owning scope if its a decl, funcScope->parent is
        //if its a definition. TODO: this is kind of stupid
        if (!call->owner->canSee(func->funcScope) && !call->owner->canSee(func->funcScope->getParent()))
            continue; //not visible in this scope

        result.push(
            std::make_pair(
                argType.compare(func->Type().getFunc().arg()),
                func)
            );
    }

    //TODO: now try template functions
    if (result.size() == 0)
    {
        err::Error(call->loc) << "function '" << oGroup->name << "' is not defined in this scope"
            << err::underline << call->loc << err::caret;
        call->Annotate(typ::error);
        return;
    }

    ovr_result firstChoice = result.top();

    if (!firstChoice.first.isValid())
    {
        err::Error(call->loc) << "no accessible instance of overloaded function '" << oGroup->name
            << "' matches arguments of type " << argType << err::underline << call->loc << err::caret;
        call->Annotate(typ::error);
    }
    else if (result.size() > 1 && (result.pop(), firstChoice.first == result.top().first))
    {
        err::Error ambigErr(call->loc);
        ambigErr << "overloaded call to '" << oGroup->name << "' is ambiguous" << err::underline
            << call->loc << err::caret << firstChoice.second->loc << err::note << "could be" << err::underline;

        while (result.size() && firstChoice.first == result.top().first)
        {
            ambigErr << result.top().second->loc << err::note << "or" << err::underline;
            result.pop();
        }
        call->ovrResult = firstChoice.second; //recover
        call->Annotate(call->ovrResult->Type().getFunc().ret());
    }
    else //success
    {
        call->ovrResult = firstChoice.second;
        call->Annotate(call->ovrResult->Type().getFunc().ret());

        //if its an intrinsic, switch it to a special node
        if (exact_cast<IntrinDeclExpr*>(call->ovrResult))
        {
            Node0* parent = call->parent;

            auto iCall = call->makeICall();

            parent->replaceDetachedChild(Ptr(iCall));

            iCall->preExec(*this);
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
        //delete the value if it becomes indeterminate
        ex.trackVal(getChildA());
    }
}

void OverloadCallExpr::preExec(Exec& ex)
{
    func->preExec(ex);
    OverloadGroupDeclExpr* oGroup = exact_cast<OverloadGroupDeclExpr*>(func->var);
    if (oGroup == 0)
    {
        err::Error(func->loc) << "cannot call object of non-function type "
            << func->Type() << err::underline;
        Annotate(func->Type()); //sorta recover
        return;
    }

    typ::TupleBuilder builder;
    for (auto& arg : Children())
    {
        arg->preExec(ex);
        builder.push_back(arg->Type(), Global().reserved.null);
    }
    typ::Type argType = typ::mgr.makeTuple(builder);

    ex.resolveOverload(oGroup, this, argType);
}

void BinExpr::preExec(Exec& ex)
{
    //FIXME: this breaks short-circuiting
    //super-FIXME: especially with val-scopes
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
        DeclExpr* def = Global().universal.getVarDef(Global().reserved.opIdents[op]);
        OverloadGroupDeclExpr* oGroup = assert_cast<OverloadGroupDeclExpr*>(def, "operator not overloaded properly");

        ex.resolveOverload(oGroup, this, argType);
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

void ReturnStmt::preExec(Exec&)
{
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

//TODO: value scopes

void IfStmt::preExec(Exec& ex)
{
    getChildA()->preExec(ex);

    if (getChildA()->Type().compare(typ::boolean) == typ::TypeCompareResult::invalid)
        err::Error(getChildA()->loc) << "expected boolean type, got " << getChildA()->Type()
            << err::underline;

    //constant...
    if (getChildA()->Value())
    {
        if (getChildA()->Value().getScalarAs<bool>()) //...and true
        {
            getChildB()->preExec(ex);
            parent->replaceChild(this, detachChildB());
        }
        else //...and false
            parent->replaceChild(this, Ptr(new NullExpr(loc)));
    }
    else
    {
        ex.pushValScope();
        getChildB()->preExec(ex);
        ex.popValScope();
        Annotate(typ::null);
    }
}

void IfElseStmt::preExec(Exec& ex)
{
    getChildA()->preExec(ex);

    if (getChildA()->Type().compare(typ::boolean) == typ::TypeCompareResult::invalid)
        err::Error(getChildA()->loc) << "expected boolean type, got " << getChildA()->Type()
            << err::underline;

    //constant...
    if (getChildA()->Value())
    {
        if (getChildA()->Value().getScalarAs<bool>()) //...and true
        {
            getChildB()->preExec(ex);
            parent->replaceChild(this, detachChildB());
        }
        else //...and false
        {
            getChildC()->preExec(ex);
            parent->replaceChild(this, detachChildC());
        }
    }
    else
    {
        ex.pushValScope();
        getChildB()->preExec(ex);
        ex.popValScope();
        
        ex.pushValScope();
        getChildC()->preExec(ex);
        ex.popValScope();

        //can't do it at runtime. unfortunately we can't tell if the value is ignored and if
        //we should emit an error
        if (getChildB()->Type().compare(getChildC()->Type()) == typ::TypeCompareResult::invalid)
            Annotate(typ::null);
        else
            Annotate(getChildB()->Type());
    }
}

//TOOD: return type and value/call?
void Exec::processFunc (ast::FuncDeclExpr* n)
{
    if (!n->Value())
        err::Error(n->loc) << "function '" << n->name << "' is never defined";
    else
    {
        FunctionDef* def = n->Value().getFunc().get();

        FuncDeclExpr* oldFunc = curFunc;
        curFunc = n;

        def->getChildA()->preExec(*this);

        for (auto ret : Subtree<ReturnStmt>(def))
            if (n->Type().getFunc().ret().compare(ret->Type()) == typ::TypeCompareResult::invalid)
                err::Error(ret->getChildA()->loc) << "cannot convert from "
                    << ret->getChildA()->Type() << " to " << n->Type().getFunc().ret()
                    << " in function return" << err::underline;

        curFunc = oldFunc;
    }
}
/*
void Sema::processSubtree (ast::Node0* n)
{
    //set the module to temporarily publicly import its own private scope.
    //this lets overloaded calls in scopes inside the public scope see
    //overloads in private or privately imported scopes
    //TODO: will this support recursive sema 3? we could track the "current" module.
    mod->pub_import.Import(&mod->priv);

    intrins.clear();
    //collect all temps set by overloadable exprs so they can be replaced
    for (auto tmp : Subtree<TmpExpr>(n))
    {
        OverloadableExpr* setBy = dynamic_cast<OverloadableExpr*>(tmp->setBy);
        if (!setBy)
            continue;
        intrins[setBy].push_back(tmp);
    }

    for (auto n : Subtree<>(n).cached())
         n->preExec(*this);

    //remove the temporary import
    mod->pub_import.UnImport(&mod->priv);
}
*/
void Exec::processMod(ast::Module* mod)
{
    //TODO: warn about cyclic imports
    mod->execStarted = true;
    for (auto import : mod->imports)
        if (!import->execStarted)
            processMod(import);

    //change functions into values so we don't enter them early
    for (auto func : Subtree<FunctionDef>(mod).cached())
    {
        Node0* parent = func->parent;
        ConstExpr* funcVal = new ConstExpr(func->loc);
        typ::Type funcType = func->Type();

        funcVal->Annotate(funcType, val::Value(func->detachSelfAs<FunctionDef>()));
        parent->replaceDetachedChild(Ptr(funcVal));
    }

    mod->getChildA()->preExec(*this);
}

Exec::Exec(ast::Module* mainMod)
{
    pushValScope();

    processMod(mainMod);

    //now find the entry point and go!!

    DeclExpr* mainVar = mainMod->priv.getVarDef(Global().reserved.main);
    if (!mainVar)
    {
        err::Error(err::fatal, mainMod->loc) << "variable 'main' undefined";
        throw err::FatalError();
    }
    OverloadGroupDeclExpr* mainFunc = exact_cast<OverloadGroupDeclExpr*>(mainVar);
    if (!mainFunc)
    {
        err::Error(err::fatal, mainVar->loc) << "variable 'main' must be a function" << err::underline;
        throw err::FatalError();
    }
    Ptr args = MkNPtr(new ConstExpr(mainFunc->loc));
    args->Annotate(typ::mgr.makeList(Global().reserved.string_t));

    OverloadCallExpr entryPt(
        MkNPtr(new VarExpr(mainFunc, mainFunc->loc)), move(args), &mainMod->priv, mainFunc->loc);

    resolveOverload(mainFunc, &entryPt, typ::mgr.makeList(Global().reserved.string_t));

    if (!entryPt.ovrResult)
    {
        err::Error(err::fatal, mainVar->loc) << "appropriate 'main' function not found" << err::underline;
        throw err::FatalError();
    }

    Global().entryPt = entryPt.ovrResult;
    processFunc(entryPt.ovrResult);
}

void Exec::pushValScope()
{
    valScopes.emplace();
}

void Exec::popValScope()
{
    for (auto var : valScopes.top())
        var->Annotate(val::Value());
    valScopes.pop();
}

void Exec::trackVal(ast::Node0* n)
{
    valScopes.top().push_back(n);
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
