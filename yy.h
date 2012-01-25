#ifndef _YY_H_
#define _YY_H_

//#pragma GCC diagnostic ignored "-Wwrite-strings"

#ifdef _MSC_VER
#define WINDOWS
#else
#define POSIX
#endif

using namespace std;

#define YYLTYPE YYLTYPE

struct YYLTYPE
{
	int first_line, last_line, first_column, last_column;
	char * filename;
	operator int() {return first_line;}
};

//pointers used in %union
class Type;
class Expr;
class Block;
class EList;
class PrimitiveType;
class TupleType;
class ListType;
class TensorType;
class Stmt;

 # define YYLLOC_DEFAULT(Current, Rhs, N)                                \
     do                                                                  \
       if (N)                                                            \
         {                                                               \
           (Current).first_line   = YYRHSLOC(Rhs, 1).first_line;         \
           (Current).first_column = YYRHSLOC(Rhs, 1).first_column;       \
           (Current).last_line    = YYRHSLOC(Rhs, N).last_line;          \
           (Current).last_column  = YYRHSLOC(Rhs, N).last_column;        \
           (Current).filename     = YYRHSLOC(Rhs, 1).filename;           \
         }                                                               \
       else                                                              \
         {                                                               \
           (Current).first_line   = (Current).last_line   =              \
             YYRHSLOC(Rhs, 0).last_line;                                 \
           (Current).first_column = (Current).last_column =              \
             YYRHSLOC(Rhs, 0).last_column;                               \
           (Current).filename     = YYRHSLOC(Rhs, 0).filename;           \
         }                                                               \
     while (0)

#define	YYDEBUG	1

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <utility>
#include <algorithm>
#include "y.tab.h"

#define IS(T,x) ((bool)dynamic_cast<T*>(x))
typedef unsigned int uint;

template <class T>
inline string to_string (const T& t)
{
	stringstream ss;
	ss << t;
	return ss.str();
}

extern int yydebug;
extern char * yytext;
extern map<string,Type*> typedefs;
extern vector<string> funcdefs;
class File;
extern File file_cur;
extern uint blocks;

extern int yyparse();
int yylex ();
void yyerror(string s);
void yywarn(string s);
void yyerror(string s, YYLTYPE l);
void yywarn(string s, YYLTYPE l);

#endif
