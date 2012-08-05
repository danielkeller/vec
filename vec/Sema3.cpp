#include "Sema.h"
#include "SemaNodes.h"
#include "Error.h"
#include "Global.h"

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
        call->Type() = typ::error;
        return;
    }

    ovr_result firstChoice = result.top();

    if (!firstChoice.first.isValid())
    {
        err::Error(call->loc) << "no accessible instance of overloaded function '" << Global().getIdent(oGroup->name)
            << "' matches arguments of type " << argType.to_str() << err::underline << call->loc << err::caret;
        call->Type() = typ::error;
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
        call->Type() = call->ovrResult->Type().getFunc().ret();
    }
    else //success
    {
        call->ovrResult = firstChoice.second;
        call->Type() = call->ovrResult->Type().getFunc().ret();
    }
}

void StringConstExpr::inferType(Sema& sema)
{
    Type() = Global().reserved.string_t;
}

void AssignExpr::inferType(Sema& sema)
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
}

void OverloadCallExpr::inferType(Sema& sema)
{
    OverloadGroupDeclExpr* oGroup = exact_cast<OverloadGroupDeclExpr*>(func->var);
    if (oGroup == 0)
    {
        err::Error(func->loc) << "cannot call object of non-function type "
            << func->Type().to_str() << err::underline;
        Type() = func->Type(); //sorta recover
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
            Type() = ft.ret();
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
        OverloadGroupDeclExpr* oGroup = exact_cast<OverloadGroupDeclExpr*>(def);
        assert(oGroup && "operator not overloaded properly");

        sema.resolveOverload(oGroup, this, argType);
    }
}

void ListifyExpr::inferType(Sema& sema)
{
    typ::Type conts_t = getChild(0)->Type();
    Type() = typ::mgr.makeList(conts_t, Children().size());
    for (auto& c : Children())
        if (conts_t.compare(c->Type()) == typ::TypeCompareResult::invalid)
            err::Error(getChild(0)->loc) << "list contents must be all the same type, " << conts_t.to_str()
                << " != " << c->Type().to_str() << err::underline << c->loc << err::underline;
}

void TuplifyExpr::inferType(Sema& sema)
{
    typ::TupleBuilder builder;
    for (auto& c : Children())
        builder.push_back(c->Type(), Global().reserved.null);
    Type() = typ::mgr.makeTuple(builder);
}

//types of exprs, for reference
//VarExpr -> DeclExpr -> FuncDeclExpr
//ConstExpr -> [IntConstExpr, FloatConstExpr, StringConstExpr] *
//BinExpr -> AssignExpr * -> OpAssignExpr
//UnExpr
//IterExpr, AggExpr
//TuplifyExpr, ListifyExpr
//PostExpr (just ++ and --)
//TupAccExpr, ListAccExpr

void Sema::processFunc (ast::Node0* n)
{
    auto tree = Subtree<>(n).preorder();
    auto treeEnd = tree.end();
    for (auto it = tree.begin(); it != treeEnd;)
    {
        if (BasicBlock * bb = exact_cast<BasicBlock*>(*it))
        {
            for (auto& it : bb->Children())
                it->inferType(*this);

            it.skipSubtree(); //don't enter function definitions, etc.
        }
        else
            ++it;
    }
}

void Sema::Phase3()
{
    //set the module to temporarily publicly import its own private scope.
    //this lets overloaded calls in scopes inside the public scope see
    //overloads in private or privately imported scopes
    //TODO: will this support recursive sema 3? we could track the "current" module.
    mod->pub_import.Import(&mod->priv);
    
    processFunc(mod);

    //remove the temporary import
    mod->pub_import.UnImport(&mod->priv);

    //now replace intrinsic calls with a special node type
    //this is kind of annoying because we have to update TmpExprs as well.
    //so we do it this way to avoid walking the tree tons of times
    std::map<OverloadableExpr*, std::list<TmpExpr*>> intrins;

    for (auto expr : Subtree<OverloadableExpr, Cast::Dynamic>(mod))
    {
        if (exact_cast<IntrinDeclExpr*>(expr->ovrResult) != nullptr)
            intrins[expr]; //add it to the map
    }

    for (auto n : Subtree<TmpExpr>(mod))
    {
        OverloadableExpr* setBy = dynamic_cast<OverloadableExpr*>(n->setBy);

        if (!setBy || !exact_cast<IntrinDeclExpr*>(setBy->ovrResult))
            continue;

        intrins[setBy].push_back(n);
    }

    for (auto it : intrins)
    {
        Node0* parent = dynamic_cast<Node0*>(it.first)->parent;

        auto iCall = it.first->makeICall();

        for (auto tmp : it.second)
            tmp->setBy = iCall.get();

        parent->replaceDetachedChild(move(iCall));
    }

    //now check stmts that want a specific type, ie return
}

NPtr<IntrinCallExpr>::type OverloadCallExpr::makeICall()
{
    IntrinDeclExpr* intrin = exact_cast<IntrinDeclExpr*>(ovrResult);
    assert(intrin && "this is not an intrin");
    return MkNPtr(new IntrinCallExpr(detachSelfAs<OverloadCallExpr>(), intrin->intrin_id));
}

NPtr<IntrinCallExpr>::type BinExpr::makeICall()
{
    IntrinDeclExpr* intrin = exact_cast<IntrinDeclExpr*>(ovrResult);
    assert(intrin && "this is not an intrin");
    return MkNPtr(new IntrinCallExpr(detachSelfAs<BinExpr>(), intrin->intrin_id));
}
