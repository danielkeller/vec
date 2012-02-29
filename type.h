#include <vector>
#include <string>
#include <utility>
using namespace std;

#define VAR -1
#define ANY -2

string mang_dim(int dim);

struct Type
{
	string name;
	virtual int size() = 0;
	virtual string c_equiv() = 0; //c version of this type
	virtual string mangle(bool n = false) = 0; //"name mangled" short form version
	virtual bool abstract() = 0; //can an object of this type be instantiated?
	virtual bool nontriv() = 0; //type is non-trivial, ie needs lib routine to use
	virtual string api_name() {return "";}
	virtual string init_stmt(string) {return "";}
//	virtual bool ref() = 0; //does this type contain reference(s) that need to be initialized?
//	virtual bool templ() = 0;
	virtual void mergep(Type *t) = 0; //replace unknowns in this type with pointers to parts of t
	virtual string to_str() = 0; //vc string representing this type
//	virtual bool simple() = 0; //trivial (ie scalar) type?
	virtual bool compatible(Type*) = 0; //can an object of type t be assigned one of this type?
//	virtual void align() = 0; //make the size of this type a multiple of 16 bytes
	virtual Type * clone() = 0; //deep copy of this type
	virtual ~Type() {};
};

//contains variable types so they may be referenced and modified/merged
//without users of the type losing track of changes
struct TypeWrapper : public Type
{
	Type * to;
	int size() {return to->size();}
	string c_equiv() {return to->c_equiv();}
	string mangle(bool n = false) {return to->mangle(n);}
	bool abstract() {return to->abstract();}
	bool nontriv() {return to->nontriv();}
	string api_name() {return to->api_name();}
	string init_stmt(string n) {return to->init_stmt(n);}
	string to_str() {return to->to_str();}
	void mergep(Type *t);
//	bool simple() {return to->simple();}
	bool compatible(Type* t);
//	void align() {to->align();}
	Type * clone() {return new TypeWrapper(to->clone());}
	TypeWrapper(Type *t) : to(t) {};
};

struct PrimitiveType : public Type
{
	enum PrimT { Int, Float } prim;
	enum SizeP { sA = ANY, s8 = 1, s16 = 2, s32 = 4, s64 = 8, sV = VAR } psize;
	int size();
	string c_equiv();
	string mangle(bool n = false) {return n && name != "" ? name : ((prim == Int ? "I" : "F") + mang_dim(psize));}
	bool abstract();
	bool nontriv() {return psize == sV;}
	string to_str();
	void mergep(Type *t);
	PrimitiveType(PrimT, SizeP);
//	bool simple() {return true;}
	bool compatible(Type*);
//	void align() {}
	Type * clone() {return new PrimitiveType(*this);}
};

struct RefType : public Type
{
	Type * to;
	int size() {return sizeof(void*);}
	string c_equiv();
	string mangle(bool n = false) {return n && name != "" ? name : ("R" + to->mangle(n));}
	void mergep(Type *t);
	bool abstract() {return false;}
	bool nontriv() {return false;}
	string to_str() {return "@" + to->to_str();}
	RefType(Type*t) : to(t) {};
//	bool simple() {return true;}
	bool compatible(Type*);
//	void align() {to->align();}
	Type * clone();
	~RefType() {delete to;}
};

struct AnyType : public Type
{
	int size() {return 1;}
	string c_equiv() {return "void";}
	string mangle(bool n = false) {return "X";}
	bool abstract() {return true;}
	bool nontriv() {return false;}
	void mergep(Type *t);
	string to_str() {return "*";}
//	bool simple() {return true;}
	bool compatible(Type*) {return true;}
//	void align() {}
	Type * clone();
};

struct ListType : public Type
{
	vector<int> dims;
	Type * contents;
	int size();
	int length();
	string c_equiv();
	string mangle(bool n = false);
	bool abstract();
	bool nontriv() {return true;}
	string api_name() {return (dims.size() && dims[0] == VAR) ? "Lv" : "Ln";}
	void mergep(Type *t);
	string to_str();
	string init_stmt(string);
	ListType() : contents(0) {};
	ListType(vector<int> d, Type* c) : dims(d), contents(c) {};
	ListType(int l, Type* c) : contents(c) {dims.push_back(l);}
//	bool simple() {return false;}
	bool compatible(Type*);
//	void align();
	Type * clone();
	~ListType() {delete contents;}
};

typedef vector<pair<string,Type*> > TupleMap;

struct TupleType : public Type
{
	TupleMap contents;
	int size();
	string c_equiv();
	string mangle(bool n = false);
	void mergep(Type *t);
	string to_str();
	bool abstract();
	bool nontriv() {return false;}
//	bool simple() {return false;}
	bool compatible(Type*);
	bool ref_compatible(TupleType*, bool); //for function args
	void align();
	Type * clone();
	~TupleType();
};

void merge(TypeWrapper * lhs, Type * rhs);
Type * demangle(string m);
