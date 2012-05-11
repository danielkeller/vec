#include "type.h"
#include "yy.h"
#include "stmt.h"
#include "tables.h"

string TypeDec(Type * type)
{
	if (IS(TypeWrapper, type))
		type = dynamic_cast<TypeWrapper*>(type)->to;

//	type->align();
	RefType * rt = dynamic_cast<RefType*>(type);
	if (rt)
		return rt->to->mangle() + "*";
//	else if (type->simple())
		return type->mangle();
//	else 
//		return type->mangle() + "*";
}

string NullStmt::output()
{
	return ";";
}

string VarDecl::output()
{
//	if (type->simple() && init)
//		return TypeDec(type->to) + " " + name + " = " + init->output() + ";";
//	else
		return TypeDec(type->to) + " " + name + ";";
}
/*
string VarDecl::alloc(int n)
{
#ifdef WINDOWS
	return type->simple() ? "" : name + " = _aligned_malloc(sizeof(*" + name
		+ ")*" + to_string(n) + ", 16);";
#else
	return type->simple() ? "" : "posix_memalign((void**)&" + name + ", 16, sizeof(*" +
		name + ")*" + to_string(n) + ");";
#endif
}*/ /*
string VarDecl::alloc(int)
{
	ListType * lt = dynamic_cast<ListType*>(type->to);
	if (lt)
		return "vsl_init_" + type->api_name() + "((" + type->api_name() + "*)&" + name + ", " +
			lt->contents->size() + ", " + lt->length + ");";
	return "";
}*/

string Func::output()
{
	string ret = TypeDec(rett) + " " + CName() + " (";
	
	if (argst->size())
	{
		ret += TypeDec(argst->contents.begin()->second) + " "
			+ argst->contents.begin()->first;
		for (TupleMap::iterator it = ++argst->contents.begin(); it != argst->contents.end(); ++it)
			ret += ", " + TypeDec(it->second) + " " + it->first;
	}

	if (conts)
		return ret + ") " + conts->output() + "\n\n";
	else
		return ret + ");";
}

string Block::output()
{
	string ret = "{";

	if (!parent)
		ret += "int __ret_flag = 0;";
	
	for (vector<VarDecl*>::iterator it = decls.begin(); it != decls.end(); ++it)
	{
		ret += (*it)->output();
		if ((*it)->type->nontriv())
			ret += (*it)->type->init_stmt((*it)->name);
	}
		
	for (vector<Stmt*>::iterator it = stmts.begin(); it != stmts.end(); ++it)
		ret += (*it)->output();
		
	ret += "epi_" + to_string(this) + ": 1;";
	
	if (parent)
		ret += "if (__ret_flag) goto epi_" + to_string(parent) + ";";
	else if (returns)
		ret += "return __ret;";

	return ret + "}\n";
}

string File::output()
{
	string ret;
	for (vector<Type*>::iterator it = types.begin(); it != types.end(); ++it)
		ret += "typedef " + (*it)->c_equiv() + " " + (*it)->mangle() + ";";
		
	ret += "\n";
	
	for (vector<Func*>::iterator it = contents.begin(); it != contents.end(); ++it)
		ret += (*it)->output();
	return ret;
}

string ReturnStmt::output()
{
	return "{__ret_flag = 1; goto epi_" + lbl + ";}";
}

string ReturnValStmt::output()
{
	return "{" + stmt->output() + "__ret_flag = 1; goto epi_" + lbl + ";}";
}

string IfStmt::output()
{
	if (econtents)
		return "if (" + cond->output() + ")\n" + contents->output() + " else\n" + econtents->output();
	else
		return "if (" + cond->output() + ")\n" + contents->output();
}

string WhileStmt::output()
{
	return "while (" + cond->output() + ")\n" + contents->output();
}

string VarExpr::output()
{
	return value;
}

string SimpleExpr::output()
{
	return sop_txt[op][Ops::before]
		+ lhs->output()
		+ sop_txt[op][Ops::between]
		+ (rhs ? rhs->output() : "")
		+ sop_txt[op][Ops::after];
}

string AssignExpr::output()
{
	if (simple)
		return "(" + lhs->output() + " = " + rhs->output() + ")";
	else
		return before + lhs->output() + between + rhs->output() + after;
}

string TupleAccExpr::output()
{
	if (idx == -2)
		return "(" + lhs->output() + ".len[0])";
	else if (idx == -1)
		return "something to get dims";
	else
		return "(" + lhs->output() + ".a" + to_string(idx) + ")";
}

string ListAccExpr::output()
{
	string ret = "(" + lhs->output() + ".a";
	for (vector<Expr*>::iterator it = rhs.begin(); it != rhs.end(); ++it)
		ret += "[" + (*it)->output() + "]";
	return ret + ")";
}

//NOTE: temp expr does not include deref, to allow & use on -ify exprs
//remember to put it in expressions that need it.
//i think this doesnt apply when everything isnt a pointer
string TmpExpr::output()
{
	return "(__tmp" + to_string(num) + ")";
};

string TuplifyExpr::output()
{ //the * and & thing is a (stupid) hack to get the , operator to return an lval.
	string ret = "*(";
	for (uint i = 0; i < exprs.size(); ++i)
		ret += tmp->output() + ".a" + to_string(i) + " = " + exprs[i]->output() + ", ";
	return ret + "&" + tmp->output() + ")";
}

string ListifyExpr::output()
{
	string ret = "*(";
	for (uint i = 0; i < exprs.size(); ++i)
		ret += tmp->output() + ".a[" + to_string(i) + "] = " + exprs[i]->output() + ", ";
	return ret + "&" + tmp->output() + ")";
}

string ConcatExpr::output()
{
	return "*vsl_" + fname + "_concat_" + fname + "(vsl_" + fname + "_assign_" + fname +
		"((" + fname + "*)&" + tmp->output() + ", (" + fname + "*)&" + lhs->output() +
		", " + to_string(sz) + "), (" + fname + "*)&" + rhs->output() + ", " +
		to_string(sz) + ")";
}

string RefExpr::output()
{
	return "(&" + to->output() + ")";
}

string DeRefExpr::output()
{
	return "(*" + to->output() + ")";
}

string CallExpr::output()
{
	string ret = func->CName() + "(";
	if (exprs.size())
	{
		ret += exprs[0]->output();
		for (uint i=1; i<exprs.size(); ++i)
			ret += ", " + exprs[i]->output();
	}
	return ret + ")";
}
