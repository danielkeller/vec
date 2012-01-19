#include "type.h"
#include "yy.h"
#include <typeinfo>

inline string dim_str (int dim)
{
	if (dim == VAR)
		return "?";
	else if (dim == ANY)
		return "*";
	else
		return to_string(dim);
}

string mang_dim(int dim)
{
	if (dim == VAR)
		return "v";
	else if (dim == ANY)
		return "a";
	else
		return to_string(dim);
}

void gen_random(char *s, const int len)
{
    static const char alphanum[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "1234567890";

    for (int i = 0; i < len; ++i)
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];

	s[0] = '_';

    s[len] = 0;
}

void merge(Type *& lhs, Type * rhs)
{
	//cout << "merge " << typeid(*lhs).name() << " " << typeid(*rhs).name() << endl;
	if (dynamic_cast<AnyType*>(lhs))
	{
		delete lhs;
		lhs = rhs;
	}
	else
		lhs->mergep(rhs);
}

void merge(TypeWrapper * lhs, Type * rhs)
{
	merge(lhs->to,rhs);
}

void TypeWrapper::mergep(Type *t)
{
	cerr << "Shouldn't be merging typewrapper" << endl;
}

bool TypeWrapper::compatible(Type* t) 
{
	TypeWrapper * tw = dynamic_cast<TypeWrapper*>(t);
	return tw ? to->compatible(tw->to) : to->compatible(t);
}

/******************************************************** Primitive */

int PrimitiveType::size()
{
	return psize;
}

string PrimitiveType::c_equiv()
{
	switch (prim)
	{
		case Int:
			switch (psize)
			{
				case s8:	return "char";
				case s16:	return "short";
				case s32:	return "long";
				case s64:	return "long long";
				case sV:	return "/*gmp variable*/ int";
				default:  return "void";
			}
		case Float:
			switch (psize)
			{
				case s32:	return "float";
				case s64:	return "double";
				case sV:	return "/*gmp variable*/ float";
				default:  return "void";
			}
		default:
			cerr << "this should not be happening" << endl;
			return "void";
	}
}

bool PrimitiveType::abstract()
{
	return psize == sA;
}

void PrimitiveType::mergep(Type *t)
{
}

string PrimitiveType::to_str()
{
	string prec;
	if (psize == sA)
		prec = "*";
	else if (psize == sV)
		prec = "?";
	else
		prec = to_string(psize * 8);
	
	if (prim == Int)
		return "int<" + prec + ">";
	else
		return "float<" + prec + ">";
}

PrimitiveType::PrimitiveType(PrimT p, SizeP s)
{
	prim = p;
	psize = s;
}

bool PrimitiveType::compatible(Type* t)
{
	PrimitiveType * pt = dynamic_cast<PrimitiveType*>(t);
	return pt && pt->prim == prim && pt->psize == psize;
}

/******************************************************** Ref */

string RefType::c_equiv()
{
	return to->c_equiv() + " *";
}

void RefType::mergep(Type *t)
{
	//anything below a ref should not be merged, we don't care about their size
	//TODO: maybe?
	//suppose we deref it later?
	//does that defeat the purpose of using a reference?

	//RefType * rt = dynamic_cast<RefType*>(t);
	//merge(to,rt->to);
}

bool RefType::compatible(Type*t)
{
	RefType * rt = dynamic_cast<RefType*>(t);
	return rt && to->compatible(rt->to);
}

Type * RefType::clone()
{
	RefType * rt = new RefType(to->clone());
	rt->name = name;
	return rt;
}

/******************************************************** Any */

Type * AnyType::clone()
{
	AnyType * rt = new AnyType();
	rt->name = name;
	return rt;
}

void AnyType::mergep(Type *t)
{
	cerr << "Shouldn't be merging anytype" << endl;
}

/******************************************************** List Type */

string ListType::c_equiv()
{
	if (length == ANY)
		return "struct { int len; " + contents->c_equiv() + "* a;}";
	else
		return "struct { " + contents->c_equiv() + " a[" + to_string(length) + "];}";
}

bool ListType::abstract()
{
	return contents->abstract();
}

void ListType::mergep(Type *t)
{
	ListType * lt = dynamic_cast<ListType*>(t);
	merge(contents, lt->contents);
}

string ListType::to_str()
{
	return "{ " + contents->to_str() + " }<" + dim_str(length) + ">";
}

bool ListType::compatible(Type * t)
{
	ListType * lt = dynamic_cast<ListType*>(t);
	return lt && (length < 0 || length == lt->length) && contents->compatible(lt->contents);
}

void ListType::align()
{
	if (length >= 0 && contents->size() * length % 16 != 0) //round up to 16b
		length += 16/contents->size() - length % (16/contents->size());
}

Type * ListType::clone()
{
 	ListType *rt = new ListType(length, contents->clone());
 	rt->name = name;
	return rt;
}

/******************************************************** Tensor */

int TensorType::size()
{
	int res = contents->size();
	for (vector<int>::iterator it = dims.begin(); it != dims.end(); it++)
		res = res * (*it);
	return res;
}

string TensorType::c_equiv()
{
	if (dims.size() == 0) //could maybe be void?
		return "struct {int dim; int * len; " + contents->c_equiv() + "* a;};"; 
	
	if (count(dims.begin(), dims.end(), ANY))
	{
		string res = "struct {int len[" + to_string(dims.size())
			+ "]; " + contents->c_equiv() + " ";
		for (int i=0; i<dims.size(); ++i)
			res += "*";
		return res + "a;}";
	}
	
	string res = "struct {" + contents->c_equiv() + " a";
	for (vector<int>::iterator it = dims.begin(); it != dims.end(); ++it)
		res += "[" + to_string(*it) + "]";
	return res + ";}";
}

string TensorType::mangle()
{
	string ret = "V";
	for (vector<int>::iterator it = dims.begin(); it != dims.end(); ++it)
		ret += to_string(*it) + "d";
	return ret;
}

bool TensorType::abstract()
{
	if (contents->abstract()) // || dims.size() == 0)
		return true;

	//if (find(dims.begin(), dims.end(), ANY) != dims.end())
	//	return true;

	return false;
}

bool TensorType::nontriv()
{
	return dims.size() == 0 || find(dims.begin(), dims.end(), ANY) != dims.end();
}

void TensorType::mergep(Type *t)
{
	TensorType * tt = dynamic_cast<TensorType*>(t);
	merge(contents, tt->contents);
}

string TensorType::to_str()
{
	string ret = "[ " + contents->to_str() + " ]<";
	if (dims.size() > 0)
	{
		ret += dim_str(*dims.begin());
		for (vector<int>::iterator it = ++dims.begin(); it != dims.end(); ++it)
			ret += ", " + dim_str(*it);
	}
	return ret + ">";
}

bool TensorType::compatible(Type*t)
{
	TensorType * tt = dynamic_cast<TensorType*>(t);
	if (!tt || !contents->compatible(tt->contents))
		return false;
	if (!dims.size())
		return true;
	if (dims.size() != tt->dims.size())
		return false;
	for (int i=0; i<dims.size(); ++i)
		if (dims[i] != ANY && dims[i] != tt->dims[i])
			return false;
	return true;
}

void TensorType::align()
{
	if (dims.size() && !count(dims.begin(), dims.end(), ANY) && contents->size() * dims.back() % 16 != 0)
		dims.back() += 16/contents->size() - dims.back() % (16/contents->size());
}

Type * TensorType::clone()
{
	TensorType * rt = new TensorType(dims, contents->clone());
	rt->name = name;
	return rt;
}

/******************************************************** Tuple */

int TupleType::size()
{
	int posn = 0;
	int largest = 0;
	for (TupleMap::iterator it = contents.begin(); it != contents.end(); ++it)
	{
		if (it->second->simple()) //align trivial types to size
		{
			if (it->second->size() > largest)
				largest = it->second->size();
			if (posn % it->second->size() != 0)
				posn += it->second->size() - posn%it->second->size();
		}
		else 
		{
			largest = 16;
			if (posn % 16 != 0) //align non trivial types to 16b
				posn += 16 - posn%16;
		}
		posn += it->second->size();
	}
	if (posn && posn % largest != 0) //make size multiple of biggest aligment
		posn += largest - posn%largest;
	return posn;
}

string TupleType::c_equiv()
{
	string ret = "struct { ";
	int n = 0;
	for (TupleMap::iterator it = contents.begin(); it != contents.end(); ++it)
	{
		ret += it->second->c_equiv() + " a" + to_string(n);
		++n;
		if (!it->second->simple()) //align non trivial types to 16b
			ret += " __attribute__ ((aligned (16)))"; //TODO: windows?
		ret += "; ";
	}
	return ret + "}";
}

string TupleType::mangle()
{
	string ret = "T";
	for (TupleMap::iterator it = contents.begin(); it != contents.end(); ++it)
		ret += it->second->mangle();
	return ret + "t";
}

bool TupleType::abstract()
{
	for (TupleMap::iterator it = contents.begin(); it != contents.end(); ++it)
		if (it->second->abstract())
			return true;
	return false;
}

void TupleType::mergep(Type *t)
{
	TupleType * tt = dynamic_cast<TupleType*>(t);
	
	for (int i=0; i < contents.size() && i < tt->contents.size(); ++i)
		merge(contents[i].second, tt->contents[i].second);
}

string TupleType::to_str()
{
	if (!contents.size())
		return "()";
	string ret = "( " + contents.begin()->second->to_str() + " " + contents.begin()->first;
	for (TupleMap::iterator it = ++contents.begin(); it != contents.end(); ++it)
		ret += ", " + it->second->to_str() + " " + it->first;
	return ret + " )";
}

bool TupleType::compatible(Type*t)
{
	TupleType * tt = dynamic_cast<TupleType*>(t);
	
	if (!tt)
		return false;

	if (contents.size() != tt->contents.size())
		return false; //this stricter type of compatibility may be changed later
	
	for (int i=0; i < contents.size() && i < tt->contents.size(); ++i)
		if (!contents[i].second->compatible(tt->contents[i].second))
			return false;
	
	return true;
}

bool TupleType::ref_compatible(TupleType*tt)
{
	if (contents.size() != tt->contents.size())
		return false;
		
	for (int i=0; i < contents.size() && i < tt->contents.size(); ++i)
	{
		RefType * rt;
		Type * tl, * tr;
		rt = dynamic_cast<RefType*>(((TypeWrapper*)contents[i].second)->to);
		tl = rt ? rt->to : contents[i].second;
		rt = dynamic_cast<RefType*>(tt->contents[i].second);
		tr = rt ? rt->to : tt->contents[i].second;

		if (!tl->compatible(tr))
			return false;
	}
	
	return true;
}

void TupleType::align()
{
	for (TupleMap::iterator it = contents.begin(); it != contents.end(); ++it)
		it->second->align();
}

Type * TupleType::clone()
{
	TupleType* ret = new TupleType();
	for (TupleMap::iterator it = contents.begin(); it != contents.end(); ++it)
		ret->contents.push_back(make_pair(it->first, it->second->clone()));
	ret->name = name;
	return ret;
}

TupleType::~TupleType()
{
	for (TupleMap::iterator it = contents.begin(); it != contents.end(); ++it)
		delete it->second;
}
