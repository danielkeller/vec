#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"
#include "Global.h"
#include "Value.h"
#include "LLVM.h"

#include <cassert>
#include <queue>

using namespace ast;
using namespace sa;

//Phase 3 is type inference, overload resolution, and template instantiation
//Additionally, we do compile time code execution. any constants are calcuated, and
//any constant, non-side-effecting expressions are removed

//convenience for overload resolution
typedef std::pair<typ::TypeCompareResult, FuncDeclExpr*> ovr_result;
struct pairComp
{
    bool operator()(const ovr_result& lhs, const ovr_result& rhs) {return rhs.first < lhs.first;}
};
typedef std::priority_queue<ovr_result, std::vector<ovr_result>, pairComp> ovr_queue;

template<class T>
void Sema::resolveOverload(OverloadGroupDeclExpr* oGroup, T* call, typ::Type argType)
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

            for (auto tmp : intrins[call])
                tmp->setBy = iCall;

            parent->replaceDetachedChild(Ptr(iCall));

            iCall->inferType(*this);
        }
    }
}

//TODO: remove constant code
void AssignExpr::inferType(Sema& sema)
{
    //try to merge types if its a decl
    if (Type().compare(getChildB()->Type()) == typ::TypeCompareResult::invalid)
    {
        //first check if we can do arith conversion
        //FIXME: this should go in copy constructor code
        if (Type().getPrimitive().isArith() && getChildB()->Type().getPrimitive().isArith())
        {
            setChildB(Ptr(new ArithCast(Type(), detachChildB())));
            getChildB()->inferType(sema);
        }
        else
            err::Error(getChildA()->loc) << "cannot convert from "
                << getChildB()->Type() << " to "
                << getChildA()->Type() << " in assignment"
                << err::underline << opLoc << err::caret
                << getChildB()->loc << err::underline;
    }
    //FIXME: this will miss some opportunities for assignment, but will prevent initializers from
    //being overwitten
    else if (getChildB()->Value() && dynamic_cast<DeclExpr*>(getChildA()))
        Annotate(getChildB()->Value());
}

void OverloadCallExpr::inferType(Sema& sema)
{
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
        builder.push_back(arg->Type(), Global().reserved.null);
    typ::Type argType = typ::mgr.makeTuple(builder);

    sema.resolveOverload(oGroup, this, argType);
}

void BinExpr::inferType(Sema& sema)
{
    if (op == tok::colon) //"normal" function call
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
                    childB->inferType(sema);
                }
                else if (rhs_t == target)
                {
                    childA = Ptr(new ArithCast(target, move(childA)));
                    childA->inferType(sema);
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

        sema.resolveOverload(oGroup, this, argType);
    }
    else
        assert("operator not yet implemented");
}

void ArithCast::inferType(Sema&)
{
    //TODO
}

void ListifyExpr::inferType(Sema&)
{
    typ::Type conts_t = getChild(0)->Type();
    Annotate(typ::mgr.makeList(conts_t, Children().size()));
    for (auto& c : Children())
        if (conts_t.compare(c->Type()) == typ::TypeCompareResult::invalid)
            err::Error(getChild(0)->loc) << "list contents must be all the same type, " << conts_t
                << " != " << c->Type() << err::underline << c->loc << err::underline;
}

void TuplifyExpr::inferType(Sema&)
{
    typ::TupleBuilder builder;
    for (auto& c : Children())
        builder.push_back(c->Type(), Global().reserved.null);
    Annotate(typ::mgr.makeTuple(builder));
}

void ReturnStmt::inferType(Sema&)
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

void IfStmt::inferType(sa::Sema&)
{
    //constant...
    if (getChildA()->Value())
    {
        if (getChildA()->Value().getScalarAs<bool>()) //...and true
            parent->replaceChild(this, detachChildB());
        else //...and false
            parent->replaceChild(this, Ptr(new NullExpr(loc)));
    }
    else
        Annotate(typ::error);
}

void IfElseStmt::inferType(sa::Sema&)
{
    //constant...
    if (getChildA()->Value())
    {
        if (getChildA()->Value().getScalarAs<bool>()) //...and true
            parent->replaceChild(this, detachChildB());
        else //...and false
            parent->replaceChild(this, detachChildC());
    }
    else
    {
        //can't do it at runtime. unfortunately we can't tell if the value is ignored and if
        //we should emit an error
        if (getChildB()->Type().compare(getChildC()->Type()) == typ::TypeCompareResult::invalid)
            Annotate(typ::error);
        else
            Annotate(getChildB()->Type());
    }
        
}

void Sema::processFunc (ast::FuncDeclExpr* n)
{
    if (!n->Value())
        err::Error(n->loc) << "function '" << n->name << "' is never defined";
    else
    {
        FunctionDef* def = n->Value().getFunc().get();

        processSubtree(def);
        for (auto ret : Subtree<ReturnStmt>(def))
            if (n->Type().getFunc().ret().compare(ret->Type()) == typ::TypeCompareResult::invalid)
                err::Error(ret->getChildA()->loc) << "cannot convert from "
                    << ret->getChildA()->Type() << " to " << n->Type().getFunc().ret()
                    << " in function return" << err::underline;
    }
}

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
         n->inferType(*this);

    //remove the temporary import
    mod->pub_import.UnImport(&mod->priv);
}

void Sema::processMod()
{
    //change functions into values so we don't enter them early
    for (auto func : Subtree<FunctionDef>(mod).cached())
    {
        Node0* parent = func->parent;
        ConstExpr* funcVal = new ConstExpr(func->loc);
        typ::Type funcType = func->Type();

        funcVal->Annotate(funcType, val::Value(func->detachSelfAs<FunctionDef>()));
        parent->replaceDetachedChild(Ptr(funcVal));
    }

    processSubtree(mod);
}

void Sema::Types()
{
    typ::mgr.makeLLVMTypes();

    processMod();

    //now find the entry point and go!!

    DeclExpr* mainVar = mod->priv.getVarDef(Global().reserved.main);
    if (!mainVar)
    {
        err::Error(err::fatal, mod->loc) << "variable 'main' undefined";
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
        MkNPtr(new VarExpr(mainFunc, mainFunc->loc)), move(args), &mod->priv, mainFunc->loc);

    resolveOverload(mainFunc, &entryPt, typ::mgr.makeList(Global().reserved.string_t));

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
