#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"
#include "Global.h"
#include "Value.h"

#include <cassert>
#include <queue>

using namespace ast;
using namespace sa;

//Phase 3 is type inference, overload resolution, and template instantiation

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
        err::Error(call->loc) << "function '" << Global().getIdent(oGroup->name) << "' is not defined in this scope"
            << err::underline << call->loc << err::caret;
        call->Annotate(typ::error);
        return;
    }

    ovr_result firstChoice = result.top();

    if (!firstChoice.first.isValid())
    {
        err::Error(call->loc) << "no accessible instance of overloaded function '" << Global().getIdent(oGroup->name)
            << "' matches arguments of type " << argType.to_str() << err::underline << call->loc << err::caret;
        call->Annotate(typ::error);
    }
    else if (result.size() > 1 && (result.pop(), firstChoice.first == result.top().first))
    {
        err::Error ambigErr(call->loc);
        ambigErr << "overloaded call to '" << Global().getIdent(oGroup->name) << "' is ambiguous" << err::underline
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

void AssignExpr::inferType(Sema&)
{
    //try to merge types if its a decl
    if (Type().compare(getChildB()->Type()) == typ::TypeCompareResult::invalid)
    {
        err::Error(getChildA()->loc) << "cannot convert from "
            << getChildB()->Type().to_str() << " to "
            << getChildA()->Type().to_str() << " in assignment"
            << err::underline << opLoc << err::caret
            << getChildB()->loc << err::underline;
        //whew!
    }
    else if (getChildB()->Value())
        Annotate(getChildB()->Value());
}

void OverloadCallExpr::inferType(Sema& sema)
{
    OverloadGroupDeclExpr* oGroup = exact_cast<OverloadGroupDeclExpr*>(func->var);
    if (oGroup == 0)
    {
        err::Error(func->loc) << "cannot call object of non-function type "
            << func->Type().to_str() << err::underline;
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
                << lhs->Type().to_str() << err::underline
                << opLoc << err::caret;
            Type() = lhs->Type(); //sorta recover
        }
        else
        {
            Annotate(ft.ret());
            if (!ft.arg().compare(getChildB()->Type()).isValid())
                err::Error(getChildA()->loc)
                    << "function arguments are inappropriate for function"
                    << ft.arg().to_str() << " != " << getChildB()->Type().to_str()
                    << err::underline << opLoc << err::caret
                    << getChildB()->loc << err::underline;
        }
    }
    else if (tok::CanBeOverloaded(op))
    {
        typ::TupleBuilder builder;
        builder.push_back(getChildA()->Type(), Global().reserved.null);
        builder.push_back(getChildB()->Type(), Global().reserved.null);
        typ::Type argType = typ::mgr.makeTuple(builder);
        DeclExpr* def = Global().universal.getVarDef(Global().reserved.opIdents[op]);
        OverloadGroupDeclExpr* oGroup = assert_cast<OverloadGroupDeclExpr*>(def, "operator not overloaded properly");

        sema.resolveOverload(oGroup, this, argType);
    }
}

void ListifyExpr::inferType(Sema&)
{
    typ::Type conts_t = getChild(0)->Type();
    Annotate(typ::mgr.makeList(conts_t, Children().size()));
    for (auto& c : Children())
        if (conts_t.compare(c->Type()) == typ::TypeCompareResult::invalid)
            err::Error(getChild(0)->loc) << "list contents must be all the same type, " << conts_t.to_str()
                << " != " << c->Type().to_str() << err::underline << c->loc << err::underline;
}

void TuplifyExpr::inferType(Sema&)
{
    typ::TupleBuilder builder;
    for (auto& c : Children())
        builder.push_back(c->Type(), Global().reserved.null);
    Annotate(typ::mgr.makeTuple(builder));
}

void Sema::processFunc (ast::FuncDeclExpr* n)
{
    if (!n->Value())
        err::Error(n->loc) << "function '" << Global().getIdent(n->name) << "' is never defined";
    else
        processSubtree(n->Value().getFunc().get());
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
