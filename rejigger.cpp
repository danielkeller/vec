#include "yy.h"
#include "type.h"
#include "stmt.h"
#include "rejigger.h"

//TODO: check if user is trying to return a reference to a local object
//TODO: test if simple expr is appropriate for operands
//TODO: optimize -ify for assignment?
//TODO: disallow 1-tuple?
//TODO: combine VAR and ANY?
//TODO: deep copies

#define ERR_BLOCK 1 //"fatal" error that renders the whole function useless
#define ERR_STMT 2 //error that makes the statement useless

#define REJ_ERR(x, e)		\
do {				\
	yyerror(x, loc);	\
	throw e;		\
} while(0)

using namespace Rejigger; //allows us to mess around with func_cur and blocks_cur

Func * Rejigger::func_cur = 0;
vector<Block*> Rejigger::blocks_cur;
vector<Stmt*> Rejigger::init_stmts;
vector<VarDecl*>::iterator Rejigger::curdecl;
vector<TmpUser*> Rejigger::tmps;
//vector<Stmt*>::iterator Rejigger::curstmt;
int tmpnum = 1;

void AddType(Type*type)
{
	type = type->clone();
	type->align();
	bool included = false;
	for(vector<Type*>::iterator it = file_cur.types.begin(); it != file_cur.types.end(); ++it)
		if ((*it)->mangle() == type->mangle())
			included = true;
	if (!included)
		file_cur.types.push_back(type);
}

Type * DoRef(Expr *& e)
{
	Type * ret = e->DoExpr();;

	if (IS(RefType,ret))
	{
		ret = dynamic_cast<RefType*>(ret)->to;
		if (!IS(RefExpr,e) && !IS(DeRefExpr,e))
			e = new DeRefExpr(e, e->loc);
	}
//	else if ((IS(VarExpr,e) && !ret->simple()))
//	{
//		e = new DeRefExpr(e, e->loc);
//	}
	return ret;
}

void TmpUser::MakeTemp(int & num, Type * t, YYLTYPE l)
{
	tmp = new TmpExpr(num++, t, l);
	tmps.push_back(this);
}

void TmpUser::MakeDecl()
{
	if (IsTmp())
		blocks_cur.front()->decls.push_back(GetTmp()->MakeDecl());
}

VarDecl * TmpExpr::MakeDecl()
{
	VarDecl * tmpdec = new VarDecl(loc);
	tmpdec->name = "__tmp" + to_string(num);
	tmpdec->type = new TypeWrapper(t);
	tmpdec->init = 0;
	tmpdec->DoStmt();
	return tmpdec;
}

Type * SimpleExpr::DoExpr()
{
	Type * ret;

	if (op != Ops::ref_assign)
	{
		ret = DoRef(lhs);
		Type * rt = DoRef(rhs);
		if (!ret->compatible(rt))
			yyerror("Expression uses incompatible types, "
				+ ret->to_str() + " != " + rt->to_str(), loc);
	}
	else
	{
		ret = lhs->DoExpr();
		RefType * lt = dynamic_cast<RefType*>(ret);
		Type * rt = rhs->DoExpr();
	
		if (lt)
		{
			if (!IS(RefType, rt) && !IS(RefExpr, rhs))
				rhs = new RefExpr(rhs, loc);
			if (!lt->compatible(rt) && !lt->to->compatible(rt))
				yyerror("Reference assignment to incompatible type, "
					+ lt->to_str() + " != " + rt->to_str(), loc);
		}
		else
			yyerror("Cannot reference-assign to a non-reference, use = instead.", loc);
	}

	return ret;
}

Type * AssignExpr::DoExpr()
{
	Type * ret = DoRef(lhs);
	Type * rt = DoRef(rhs);
	if (!ret->compatible(rt))
		REJ_ERR("Expression uses incompatible types, "
			+ ret->to_str() + " != " + rt->to_str(), ERR_STMT);

	if (!lhs->lval())
		yyerror("Cannot assign to non-lvalue", loc);
	if (ret->nontriv())
	{
		simple = false;
		before = "*vsl_" + ret->api_name() + "_assign_" + rt->api_name() + "(("
			+ ret->api_name() + "*)&";
		between = ", (" + rt->api_name() + "*)&";
		after = ", ";
		if (IS(ListType, rt))
		{
			ListType * lrt = dynamic_cast<ListType*>(rt);
			after += to_string(lrt->contents->size());
		}
		after += ")";
	}
	return ret;
}

Type * TupleAccExpr::DoExpr()
{
	Type * t = DoRef(lhs);
	TupleType * tt = dynamic_cast<TupleType*>(t);
	if (!tt && rhs == "length") //tuple access special case
	{
		idx = -1;
		if (IS(ListType, t))
			return new PrimitiveType(PrimitiveType::Int, PrimitiveType::s32);
		TensorType * te = dynamic_cast<TensorType*>(t);
		if (te)
			return new ListType(te->dims.size(), new PrimitiveType(PrimitiveType::Int, PrimitiveType::s32));

		REJ_ERR("Cannot retrieve length of non- list or tensor type.", ERR_STMT);
	}
	else if (!tt)
		REJ_ERR("Operand not of tuple type", ERR_STMT);

	uint i=0;
	for (; i < tt->contents.size(); ++i)
		if (tt->contents[i].first == rhs)
			break;
	
	if (i == tt->contents.size())
		REJ_ERR("Tuple does not contain element '" + rhs + "'", ERR_STMT);
	idx = i;

	return tt->contents[i].second;
}

Type * TensorAccExpr::DoExpr()
{
	for (vector<Expr*>::iterator it = rhs.begin(); it != rhs.end(); ++it)
	{
		PrimitiveType * rt = dynamic_cast<PrimitiveType*>((*it)->DoExpr());
		if (!rt || rt->prim != PrimitiveType::Int)
			yyerror("Tensor index must be of integral type", loc);
	}

	TensorType * ret = dynamic_cast<TensorType*>(DoRef(lhs));

	if(ret)
	{
		if (rhs.size() != ret->dims.size())
			yyerror("Incorrect number of dimensions for tensor access, expected " 
				+ to_string(ret->dims.size()) + ", got " + to_string(rhs.size()), loc);
		return ret->contents;
	}
	else
		REJ_ERR("Operand not of tensor type", ERR_STMT);
}

Type * ListAccExpr::DoExpr()
{
	PrimitiveType * rt = dynamic_cast<PrimitiveType*>(rhs->DoExpr());
	if (!rt || rt->prim != PrimitiveType::Int)
		yyerror("List index must be of integral type", loc);

	ListType * ret = dynamic_cast<ListType*>(DoRef(lhs));

	if (ret)
		return ret->contents;
	else
		REJ_ERR("Operand not of list type", ERR_STMT);
}

Type * TuplifyExpr::DoExpr()
{
	TupleType * t = new TupleType();
	for(uint i = 0; i < exprs.size(); ++i)
		t->contents.push_back(make_pair("a" + to_string(i), DoRef(exprs[i])));

	if (tmp) //don't make extra temps on further DoExpr passes
		return t;
	
	MakeTemp(tmpnum, t, loc);

	return t;
}

Type * ListifyExpr::DoExpr()
{
	ListType * t = new ListType(exprs.size(), DoRef(exprs[0]));
	for(uint i = 1; i < exprs.size(); ++i)
		if (!t->contents->compatible(DoRef(exprs[i])))
			yyerror("Types in listify are not compatible: "
				+ t->contents->to_str() + " != " + DoRef(exprs[i])->to_str(), loc);

	if (tmp) //don't make extra temps on further DoExpr passes
		return t;
	
	MakeTemp(tmpnum, t, loc);
	
	return t;
}

Type * ConcatExpr::DoExpr()
{
	return 0;
}

Type * RefExpr::DoExpr()
{
	return to->DoExpr();
}

Type * DeRefExpr::DoExpr()
{
	return to->DoExpr();
}

Type * CallExpr::DoExpr()
{
	TupleType * t = new TupleType();
	for(uint i = 0; i < exprs.size(); ++i)
		t->contents.push_back(make_pair("", exprs[i]->DoExpr()));

//	string mname = name + "$" + t->mangle();
	for (vector<Func*>::iterator it = file_cur.contents.begin(); it != file_cur.contents.end(); ++it)
		if ((*it)->name == name && (*it)->argst->ref_compatible(t))
//		if ((*it)->mname == mname) 
			func = *it;
	
	if (!func)
		REJ_ERR("No compatible function declared: " + name + ":" + t->to_str(), ERR_STMT);
	
	//auto ref-ify args
	for (uint i = 0; i < exprs.size(); ++i)
	{
		if (IS(RefType, ((TypeWrapper*)func->argst->contents[i].second)->to) && !IS(RefType, t->contents[i].second))
			exprs[i] = new RefExpr(exprs[i], loc);
		else if (!IS(RefType, ((TypeWrapper*)func->argst->contents[i].second)->to) && IS(RefType, t->contents[i].second))
			exprs[i] = new DeRefExpr(exprs[i], loc);
	}

	return func->rett->to;
}

void ExprStmt::DoStmt()
{
	ex->DoExpr();
}

void ReturnStmt::DoStmt()
{
	lbl = to_string(blocks_cur.back());
}

void ReturnValStmt::DoStmt()
{
	Type * rett = val->DoExpr();

	lbl = to_string(blocks_cur.back());
	if (!func_cur->rett->compatible(rett))
		REJ_ERR("Expression incompatible with return type: "
			+ func_cur->rett->to_str() + " != " + rett->to_str(), ERR_STMT);
	merge(func_cur->rett, rett);

	if (!resolve("__ret"))
	{
		VarDecl * d = new VarDecl(loc);
		d->type = func_cur->rett;
		d->name = "__ret";//V uses FRONT, not BACK
		d->DoStmt();
		blocks_cur.front()->decls.push_back(d);
	}

	stmt = new ExprStmt(new AssignExpr(
		new VarExpr("__ret", func_cur->rett, loc), val, loc), loc);

	stmt->DoStmt();
}

void IfStmt::DoStmt()
{
	cond->DoExpr();
	contents->DoStmt();
	if (econtents)
		econtents->DoStmt();
}

void WhileStmt::DoStmt()
{
	cond->DoExpr();
	contents->DoStmt();
}

void VarDecl::DoStmt()
{
	
	if (type->nontriv())
	{
		ListType * lt = dynamic_cast<ListType*>(type->to);
		if (lt)
		{
			init_stmts.push_back(new ExprStmt(new ConstExpr("vsl_init_" + type->api_name() +
				"((" + type->api_name() + "*)&" + name + ", " + to_string(lt->contents->size())
				+ ", " + to_string(lt->length < 0 ? 0 : lt->length) + ")", type->to, loc), loc));
		}
	}

	if (init)
	{
		Type * exprt = init->DoExpr();
		
		RefType *rt = dynamic_cast<RefType*>(type->to);
		if (rt)
		{
			if (rt->compatible(exprt))
			{
				//do nothing
			}
			else if (rt->to->compatible(exprt))
			{
				if (!init->lval())
				{
					yyerror("Cannot referece a non-lval expression", loc);
					init = 0;
				}
				else if (rt->to->simple() && !IS(RefExpr, init))
					init = new RefExpr(init, loc);
				else //initializer must be added after ref'd object is allocated
				{
					init_stmts.push_back(new ExprStmt(new SimpleExpr(Ops::ref_assign,
						new VarExpr(name, type, loc), init, loc), loc));
					init = 0;
				}
			}
			else
			{
				yyerror("Reference initializer for '" + name + "' is incompatible with its type: " +
					rt->to_str() + " != " + exprt->to_str(), loc);
				init = 0; //brute force error recovery
			}
		}
		else if (type->compatible(exprt))
		{
			merge(type, exprt);
			if (type->abstract())
			{
				yyerror("Initializer for '" + name + "' does not provide enough"
					" information to fully determine its type, got: " + type->to_str(), loc);
			//	return; //can't do any more here
			}
		}
		else
		{
			yyerror("Initializer for '" + name + "' is incompatible with its type: " +
				type->to_str() + " != " + exprt->to_str(), loc);
			init = 0; //brute force error recovery
		}
		
		if (init) //recheck for init
		{
			if (IS(RefType, type->to))
				init_stmts.push_back(new ExprStmt(new SimpleExpr(Ops::ref_assign,
					new VarExpr(name, type, loc), init, loc), loc));
			else
				init_stmts.push_back(new ExprStmt(new AssignExpr(
					new VarExpr(name, type, loc), init, loc), loc));
			init = 0;
		}
	}
}

void Block::DoStmt()
{
	blocks_cur.push_back(this);
	for (curdecl = decls.begin(); curdecl != decls.end(); ++curdecl)
	{
		try
		{
			(*curdecl)->DoStmt();
		}
		catch (int e)
		{
			if (e == ERR_BLOCK)
				throw;
		}
	}
	
	//insert initialization stmts from normal decls before DoStmt'ing all stmts
	stmts.insert(stmts.begin(), init_stmts.begin(), init_stmts.end());
	init_stmts.clear();

	for (vector<Stmt*>::iterator curstmt = stmts.begin(); curstmt != stmts.end(); ++curstmt)
	{
		try
		{
			(*curstmt)->DoStmt();
		}
		catch (int e)
		{
			if (e == ERR_BLOCK)
				throw;
		}
	}

	//add temporary declarations for any stmt that needs it. MakeDecl calls DoStmt
	for (vector<TmpUser*>::iterator it = tmps.begin(); it != tmps.end(); ++it)
		(*it)->MakeDecl();
	
	//insert init statements for temp declarations
	stmts.insert(stmts.begin(), init_stmts.begin(), init_stmts.end());
	init_stmts.clear();

	for (vector<VarDecl*>::iterator it = decls.begin(); it != decls.end(); ++it)
	{
		RefType * rt = dynamic_cast<RefType*>((*it)->type->to);
		AddType(rt ? rt->to : (*it)->type->to);
	}


	if (blocks_cur.size() == 1) //check for return value
		if (!IS(AnyType, func_cur->rett->to))
		{
			if (!resolve("__ret"))
				yyerror("Function '" + func_cur->name + "' must return a value", func_cur->loc);
			else
				returns = true;
		}

	blocks_cur.pop_back();

//	if (blocks_cur.size()) //restore curstmt iterator
//		curstmt = find(blocks_cur.back()->stmts.begin(), blocks_cur.back()->stmts.end(), this);
}

void Func::DoStmt()
{
//	if (cexp)
//		mname = name;
//	else
		mname = name + "$" + argst->mangle();

	if (!conts)
	{
/*		for (vector<Func*>::iterator it = file_cur.contents.begin(); it != file_cur.contents.end(); ++it)
			if ((*it) != this && !(*it)->conts && (*it)->mname == mname)
				REJ_ERR("Redeclaration of function " + rett->to_str() +
					":" + argst->to_str() + " " + name, ERR_BLOCK);
*/
	}
	else
	{
		for (vector<Func*>::iterator it = file_cur.contents.begin(); it != file_cur.contents.end(); ++it)
//			if ((*it) != this && (*it)->conts && (*it)->name == name && (*it)->argst->compatible(argst))
			if ((*it) != this && (*it)->conts && (*it)->mname == mname)
				REJ_ERR("Redefinition of function " + rett->to_str() +
					":" + argst->to_str() + " " + name, ERR_BLOCK);

		func_cur = this;
		conts->DoStmt();
	}

	AddType(rett);
	for (TupleMap::iterator it = argst->contents.begin(); it != argst->contents.end(); ++it)
	{
		AddType(it->second);
	}
}

void Rejigger::Clear()
{
	blocks_cur.clear();
	init_stmts.clear();
	tmps.clear();
	func_cur = 0;
}

bool Rejigger::DoFunction(Func * f)
{
	try {
	f->DoStmt();
	} catch (...) {
		Clear();
		return false;
	}
	Clear();
	return true;
}

TypeWrapper * Rejigger::resolve(string name)
{
	for (vector<Block*>::reverse_iterator it = blocks_cur.rbegin(); it != blocks_cur.rend(); ++it)
		for (vector<VarDecl*>::iterator it2 = (*it)->decls.begin(); it2 != (*it)->decls.end(); ++it2)
			if ((*it2)->name == name)
				return (*it2)->type;
	
	for (TupleMap::iterator it = func_cur->argst->contents.begin(); it != func_cur->argst->contents.end(); ++it)
		if (it->first == name)
			return dynamic_cast<TypeWrapper*>(it->second);
	
	return 0;
}
