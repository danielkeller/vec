#include <vector>
#include "tables.h"
#include "yy.h"

struct Stmt
{
	YYLTYPE loc;
	virtual string output() = 0;
	virtual void DoStmt() {};
	//virtual string output(int n) = 0;
};

struct NullStmt : public Stmt
{
	string output();
	void DoStmt() {}
	NullStmt(YYLTYPE l) {loc = l;}
};

struct VarDecl : public Stmt
{
	string name;
	TypeWrapper * type;
	Expr * init;
	string output();
	string alloc(int n);
	VarDecl(YYLTYPE l) : type(0) {loc = l; init = 0;}
	void DoStmt();
};

struct Func : public Stmt
{
	Block * conts;
	TupleType * argst;
	TypeWrapper * rett;
	string name;
	string mname;
	bool cexp;
	string output();
	void DoStmt();
	string CName() {return cexp ? name : mname;}
	Func() : conts(0), argst(0), rett(0), cexp(false) {}
	Func(TupleType *a, TypeWrapper *r, string n, YYLTYPE l) : conts(0), argst(a), rett(r), name(n), cexp(false) {loc = l;}
};

struct Block : public Stmt
{
	vector<VarDecl*> decls;
	vector<Stmt*> stmts;
	Block * parent;
	bool returns;
	string output();
	void DoStmt();
	Block(YYLTYPE l) {loc = l;}
};

struct File : public Stmt
{
	vector<Func*> contents;
	vector<Type*> types;
	string output();
	string output(int n) {return output();}
};

struct ReturnStmt : public Stmt
{
	string lbl;
	string output();
	void DoStmt();
	ReturnStmt(YYLTYPE l) {loc = l;}
};
	
struct Expr
{
	YYLTYPE loc;
	virtual string output() = 0;
	virtual Type * DoExpr() = 0;
	virtual bool lval() = 0;
};

struct ExprStmt : public Stmt
{
	Expr * ex;
	string output() {return ex ? ex->output() + ";" : ";";}
	void DoStmt();
	ExprStmt(Expr * e, YYLTYPE l) : ex(e) {loc = l;}
};

struct ReturnValStmt : public Stmt
{
	string lbl;
	Expr * val;
	ExprStmt * stmt;
	string output();
	void DoStmt();
	ReturnValStmt(Expr * v, YYLTYPE l) : val(v) {loc = l;}
};

struct IfStmt : public Stmt
{
	Expr * cond;
	Stmt * contents;
	Stmt * econtents;
	string output();
	void DoStmt();
	IfStmt(Expr * c, Stmt * cont, YYLTYPE l) : cond(c), contents(cont), econtents(0) {loc = l;}
	IfStmt(Expr * c, Stmt * cont, Stmt * e, YYLTYPE l) : cond(c), contents(cont), econtents(e) {loc = l;}
};

struct WhileStmt : public Stmt
{
	Expr * cond;
	Stmt * contents;
	string output();
	void DoStmt();
	WhileStmt(Expr * c, Stmt * cont, YYLTYPE l) : cond(c), contents(cont) {loc = l;}
};

struct ConstExpr : public Expr
{
	string value;
	Type * type;
	string output() {return value;}
	Type * DoExpr() {return type;}
	bool lval() {return false;}
	ConstExpr(string v, Type *t, YYLTYPE l) : value(v), type(t) {loc = l;}
};

struct VarExpr : public Expr
{
	string value;
	TypeWrapper * type;
	string output();
	Type * DoExpr() {return type->to;}
	bool lval() {return true;}
	VarExpr(string v, TypeWrapper *t, YYLTYPE l) : value(v), type(t) {loc = l;}
};

//also conversion, horizontal

struct SimpleExpr : public Expr
{
	Expr * lhs;
	Expr * rhs;
	Ops::Sop op;
	string output();
	Type * DoExpr();
	bool lval() {return false;}
	SimpleExpr(Ops::Sop o, Expr * l, Expr * r, YYLTYPE lo) : lhs(l), rhs(r), op(o) {loc = lo;}
};

struct AssignExpr : public Expr
{
	Expr * lhs;
	Expr * rhs;
	bool simple;
	string before;
	string between;
	string after;
	string output();
	Type * DoExpr();
	bool lval() {return true;}
	AssignExpr(Expr * l, Expr * r, YYLTYPE lo) : lhs(l), rhs(r), simple(true) {loc = lo;}
};

struct TupleAccExpr : public Expr
{
	Expr * lhs;
	string rhs;
	int idx;
	string output();
	Type * DoExpr();
	bool lval() {return lhs->lval() && idx >= 0;}
	TupleAccExpr(Expr * l, string r, YYLTYPE lo) : lhs(l), rhs(r) {loc = lo;}
};

struct ListAccExpr : public Expr
{
	Expr * lhs;
	vector<Expr*> rhs;
	string output();
	Type * DoExpr();
	bool lval() {return lhs->lval();}
	ListAccExpr(Expr * l, YYLTYPE lo) : lhs(l) {loc = lo;}
};

struct TmpExpr : public Expr
{
	int num;
	Type * t;
	string output();
	Type * DoExpr() {return t;}
	bool lval() {return true;}
	VarDecl * MakeDecl();
	TmpExpr(int n, Type * t, YYLTYPE l) : num(n), t(t) {loc = l;}
};
	
struct EList
{
	vector<Expr*> exprs;
};

struct TmpUser
{
	Expr * tmp;
	bool IsTmp() {return IS(TmpExpr, tmp);}
	inline TmpExpr * GetTmp() {return dynamic_cast<TmpExpr*>(tmp);}
	void MakeTemp(int &num, Type *t, YYLTYPE l);
	void MakeDecl();
	TmpUser() : tmp(0) {}
};

struct TuplifyExpr : public Expr, public EList , public TmpUser
{
	string output();
	Type * DoExpr();
	bool lval() {return false;}
	TuplifyExpr(EList & li, YYLTYPE l) : EList(li) {loc = l;}
};

struct ListifyExpr : public Expr, public EList , public TmpUser
{
	string output();
	Type * DoExpr();
	bool lval() {return false;}
	ListifyExpr(EList & li, YYLTYPE l) : EList(li) {loc = l;}
};

struct ConcatExpr : public Expr, public TmpUser
{
	Expr *lhs, *rhs;
	string fname;
	int sz;
	string output();
	Type * DoExpr();
	bool lval() {return false;}
	ConcatExpr(Expr * l, Expr * r, YYLTYPE lo) : lhs(l), rhs(r) {loc = lo;}
};

struct RefExpr : public Expr
{
	Expr * to;
	string output();
	Type * DoExpr();
	bool lval() {return to->lval();}
	RefExpr(Expr *e, YYLTYPE l) : to(e) {loc = l;}
};

struct DeRefExpr : public Expr
{
	Expr * to;
	string output();
	Type * DoExpr();
	bool lval() {return to->lval();}
	DeRefExpr(Expr *e, YYLTYPE l) : to(e) {loc = l;}
};

struct CallExpr : public Expr, public EList
{
	string name;
	Func * func;
	string output();
	Type * DoExpr();
	bool lval() {return false;}
	CallExpr(string n, YYLTYPE l) : name(n) {loc = l;}
	CallExpr(string n, EList & li, YYLTYPE l) : EList(li), name(n) {loc = l;}
};
