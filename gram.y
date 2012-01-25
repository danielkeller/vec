%{
#include "yy.h"
#include "type.h"
#include "stmt.h"
#include "tables.h"
#include "rejigger.h"

uint yyerrno = 0;
map<string,Type*> typedefs;
vector<string> funcdefs;
File file_cur;
Func * func_cur = 0;
vector<Block*> blocks_cur;
uint blocks = 0;

struct TypeWrapper;
TypeWrapper * resolve(string name);
%}

%locations
%defines
%error-verbose

%union
{
	string * txt_v;
	long long long_v;
	Stmt * stmt_v;
	Block * block_v;
	Expr * expr_v;
	EList * elst_v;
	Type * type_v;
	PrimitiveType * prim_v;
	ListType * list_v;
	TupleType * tup_v;
	TensorType * tens_v;
};

%token <txt_v> IDENTIFIER "identifier" TYPE_NAME "type name" FUNC_NAME "function name" STRING_LITERAL "string literal"
%token <long_v> CONSTANT_INT "constant int"
%token <txt_v> CONSTANT_FLOAT "constant float"
%token INC_OP "++" DEC_OP "--" LEFT_OP "<<" RIGHT_OP ">>" LE_OP "<=" GE_OP ">=" EQ_OP "==" NE_OP "!="
%token AND_OP "&&" OR_OP "||" MUL_ASSIGN "*=" DIV_ASSIGN "/=" MOD_ASSIGN "%=" ADD_ASSIGN "+="
%token SUB_ASSIGN "-=" LEFT_ASSIGN "<<=" RIGHT_ASSIGN ">>=" AND_ASSIGN "&&="
%token XOR_ASSIGN "^=" OR_ASSIGN "|=" REF_ASSIGN "@="

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%token INT FLOAT
%token TYPEDEF "type" INLINE CIMP CEXP

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%type<txt_v> function_name
%type<type_v> type func_ret
%type<prim_v> primitive primitive_type
%type<list_v> list_type
%type<tup_v> tuple_list tuple_type
%type<tens_v> dim_list tensor_type
%type<expr_v> primary_expression postfix_expression unary_expression multiplicative_expression additive_expression shift_expression relational_expression equality_expression and_expression exclusive_or_expression inclusive_or_expression concat_expression logical_and_expression logical_or_expression conditional_expression expression
%type<elst_v> expression_list shift_expression_list
%type<stmt_v> statement expression_statement jump_statement selection_statement iteration_statement
%type<block_v> compound_statement

%start translation_unit

%%
/******************************************************** Expressions and Operators */

primary_expression
	: IDENTIFIER
	{
		TypeWrapper * t = resolve(*$1);
		if (!t)
		{
			yyerror("Symbol '" + *$1 + "' not declared in this scope");
			YYERROR; //raise an error as this expression is useless now
		}
		$$ = new VarExpr(*$1, t, @1);
		delete $1;
	}
	| CONSTANT_INT
	{
		PrimitiveType * t;
		char * end = &yytext[strlen(yytext)-1];
		switch (*end)
		{
			case 'l':
			case 'L':
				t = new PrimitiveType(PrimitiveType::Int, PrimitiveType::s64);
				*end = 0;
				break;
			case 's':
			case 'S':
				t = new PrimitiveType(PrimitiveType::Int, PrimitiveType::s16);
				*end = 0;
				break;
			case 'c':
			case 'C':
				t = new PrimitiveType(PrimitiveType::Int, PrimitiveType::s8);
				*end = 0;
				break;
			default:
				t = new PrimitiveType(PrimitiveType::Int, PrimitiveType::s32);
				break;
		}
		$$ = new ConstExpr(yytext + to_string("ll"), t, @1);
	}
	| CONSTANT_FLOAT
	{
		if (yytext[strlen(yytext)-1] == 'l')
			$$ = new ConstExpr(yytext, new PrimitiveType(PrimitiveType::Float, PrimitiveType::s64), @1);
		else
			$$ = new ConstExpr(yytext, new PrimitiveType(PrimitiveType::Float, PrimitiveType::s32), @1);
		delete $1;
	}
	| STRING_LITERAL
	{

	}
	| '(' expression_list ')'
	{
		if ($2->exprs.size() == 1)
			$$ = $2->exprs[0];
		else
			$$ = new TuplifyExpr(*$2, @1);

		delete $2;
	}
	| '[' expression_list ']'
	| '{' expression_list '}'
	{
		$$ = new ListifyExpr(*$2, @1);
		delete $2;
	}
	| '[' ']' '<' shift_expression_list '>' //it's stupid to have to do it this way. i should
	| '{' '}' '<' shift_expression '>' // find a better syntax for this
	| FUNC_NAME ':'  '(' ')'
	{
		$$ = new CallExpr(*$1, @1);
		delete $1;
	}
	| FUNC_NAME  ':' '(' expression_list ')'
	{
		$$ = new CallExpr(*$1, *$4, @1);
		delete $1;
		delete $4;
	}
	;

shift_expression_list
	: shift_expression
	{
		$$ = new EList();
		$$->exprs.push_back($1);
	}
	| shift_expression_list ',' shift_expression
	{
		$$ = $1;
		$$->exprs.push_back($3);
	}
	;

expression_list
	: expression
	{
		$$ = new EList();
		$$->exprs.push_back($1);
	}
	| expression_list ',' expression
	{
		$$ = $1;
		$$->exprs.push_back($3);
	}
	;

postfix_expression
	: primary_expression
	| postfix_expression '[' expression_list ']'
	{
		TensorAccExpr * e = new TensorAccExpr($1,@2);
		e->rhs = $3->exprs;
		$$ = e;
		delete $3;
	}
	| postfix_expression '(' IDENTIFIER ')'
	{
		$$ = new TupleAccExpr($1, *$3, @2);
		delete $3;
	}
	| postfix_expression '{' expression '}'
	{
		$$ = new ListAccExpr($1, $3, @2);
	}
	| postfix_expression INC_OP
	{

	}
	| postfix_expression DEC_OP
	{

	}
	;

unary_expression
	: postfix_expression
	| INC_OP postfix_expression
	{

	}
	| DEC_OP postfix_expression
	{

	}
	| '-' postfix_expression
	{
		$$ = new SimpleExpr(Ops::neg, $2, 0, @1);
	}
	| '~' postfix_expression
	| '!' postfix_expression
	;

multiplicative_expression
	: unary_expression
	| multiplicative_expression '*' unary_expression
	{
		$$ = new SimpleExpr(Ops::times, $1, $3, @2);
	}
	| multiplicative_expression '/' unary_expression
	{
		$$ = new SimpleExpr(Ops::div, $1, $3, @2);
	}
	| multiplicative_expression '%' unary_expression
	{
	}
	;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression
	{
		$$ = new SimpleExpr(Ops::plus, $1, $3, @2);
	}
	| additive_expression '-' multiplicative_expression
	{
		$$ = new SimpleExpr(Ops::minus, $1, $3, @2);
	}
	;

shift_expression
	: additive_expression
	| shift_expression LEFT_OP additive_expression
	| shift_expression RIGHT_OP additive_expression
	;

relational_expression
	: shift_expression
	| relational_expression '<' shift_expression
	{
		$$ = new SimpleExpr(Ops::lt, $1, $3, @2);
	}
	| relational_expression '>' shift_expression
	{
		$$ = new SimpleExpr(Ops::gt, $1, $3, @2);
	}
	| relational_expression LE_OP shift_expression
	{
		$$ = new SimpleExpr(Ops::le, $1, $3, @2);
	}
	| relational_expression GE_OP shift_expression
	{
		$$ = new SimpleExpr(Ops::ge, $1, $3, @2);
	}
	;

equality_expression
	: relational_expression
	| equality_expression EQ_OP relational_expression
	{
		$$ = new SimpleExpr(Ops::eq, $1, $3, @2);
	}
	| equality_expression NE_OP relational_expression
	{
		$$ = new SimpleExpr(Ops::ne, $1, $3, @2);
	}
	;

and_expression
	: equality_expression
	| and_expression '&' equality_expression
	{
		$$ = new SimpleExpr(Ops::band, $1, $3, @2);
	}
	;

exclusive_or_expression
	: and_expression
	| exclusive_or_expression '^' and_expression
	{
		$$ = new SimpleExpr(Ops::bxor, $1, $3, @2);
	}
	;

inclusive_or_expression
	: exclusive_or_expression
	| inclusive_or_expression '|' exclusive_or_expression
	{
		$$ = new SimpleExpr(Ops::bor, $1, $3, @2);
	}
	;

concat_expression
	: inclusive_or_expression
	| concat_expression '$' inclusive_or_expression
	{
		$$ = new ConcatExpr($1, $3, @2);
	}
	;

logical_and_expression
	: concat_expression
	| logical_and_expression AND_OP inclusive_or_expression
	{
		$$ = new SimpleExpr(Ops::land, $1, $3, @2);
	}
	;

logical_or_expression
	: logical_and_expression
	| logical_or_expression OR_OP logical_and_expression
	{
		$$ = new SimpleExpr(Ops::lor, $1, $3, @2);
	}
	;

conditional_expression
	: logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression
	;

expression
	: conditional_expression
	| unary_expression '=' expression
	{
		$$ = new AssignExpr($1, $3, @2);
	}
	| unary_expression REF_ASSIGN expression
	{
		$$ = new SimpleExpr(Ops::ref_assign, $1, $3, @2);
	}
	| unary_expression MUL_ASSIGN expression
	{
		$$ = new AssignExpr($1,  new SimpleExpr(Ops::times, $1, $3, @2), @2);
	}
	| unary_expression DIV_ASSIGN expression
	{
		$$ = new AssignExpr($1,  new SimpleExpr(Ops::div, $1, $3, @2), @2);
	}
	| unary_expression MOD_ASSIGN expression
	| unary_expression ADD_ASSIGN expression
	{
		$$ = new AssignExpr($1,  new SimpleExpr(Ops::plus, $1, $3, @2), @2);
	}
	| unary_expression SUB_ASSIGN expression
	{
		$$ = new AssignExpr($1,  new SimpleExpr(Ops::minus, $1, $3, @2), @2);
	}
	| unary_expression LEFT_ASSIGN expression
	| unary_expression RIGHT_ASSIGN expression
	| unary_expression AND_ASSIGN expression
	{
		$$ = new AssignExpr($1,  new SimpleExpr(Ops::band, $1, $3, @2), @2);
	}
	| unary_expression XOR_ASSIGN expression
	{
		$$ = new AssignExpr($1,  new SimpleExpr(Ops::bxor, $1, $3, @2), @2);
	}
	| unary_expression OR_ASSIGN expression
	{
		$$ = new AssignExpr($1,  new SimpleExpr(Ops::bor, $1, $3, @2), @2);
	}
	;
	

/******************************************************** Types */

primitive
	: INT
	{
		$$ = new PrimitiveType(PrimitiveType::Int,PrimitiveType::s32);
	}
	| FLOAT
	{
		$$ = new PrimitiveType(PrimitiveType::Float,PrimitiveType::s32);
	}
	;

primitive_type
	: primitive /*using default size - int<32>, float<32> */
	{
		$$ = $1;
	}
	| primitive '<' CONSTANT_INT '>'
	{
		if ($3 == 32 || $3 == 64 || (($3 == 8 || $3 == 16) && $1->prim == PrimitiveType::Int))
			$1->psize = (PrimitiveType::SizeP)($3/8);
		else
		{
			if ($1->prim == PrimitiveType::Int)
				yyerror("Invalid precision " + to_string($3) + ", may be 8, 16, 32, 64, ?, *");
			else
				yyerror("Invalid precision " + to_string($3) + ", may be 32, 64, ?, *");
		}
		
	}
	| primitive '<' '?' '>'
	{
		$1->psize = PrimitiveType::sV;
		$$ = $1;
	}
	| primitive '<' '*' '>'
	{
		$1->psize = PrimitiveType::sA;
		$$ = $1;
	}
	;

list_type
	: '{' type '}' '<' CONSTANT_INT '>'
	{
		$$ = new ListType($5, $2);
	}
	| '{' type '}' '<' '?' '>'
	{
		$$ = new ListType(VAR, $2);
	}
	| '{' type '}' '<' '*' '>'
	{
		$$ = new ListType(ANY, $2);
	}
	;

tensor_type
	: '[' type ']' '<' dim_list '>'
	{
		$$ = $5;
		$$->contents = $2;
	}
	| '[' type ']' '<' '>'
	{
		$$ = new TensorType();
		$$->contents = $2;
	}
	;

dim_list
	: CONSTANT_INT
	{
		$$ = new TensorType();
		$$->dims.push_back($1);
	}
	| '*'
	{
		$$ = new TensorType();
		$$->dims.push_back(ANY);
	}
	| dim_list ',' CONSTANT_INT
	{
		$$ = $1;
		$$->dims.push_back($3);
	}
	| dim_list ',' '*'
	{
		$$ = $1;
		$$->dims.push_back(ANY);
	}
	;

tuple_type
	: '(' tuple_list ')'
	{
		$$ = $2;
	}
	| '(' ')'
	{
		$$ = new TupleType();
	}
	/*| '(' tuple_list  '.' '.' '.' ')'*/ /* this may not be all that useful */
	;

tuple_list
	: type IDENTIFIER
	{
		$$ = new TupleType();
		$$->contents.push_back(make_pair(*$2, $1));
		delete $2;
	}
	| type
	{
		$$ = new TupleType();
		$$->contents.push_back(make_pair("", $1));
	}
	| tuple_list ',' type IDENTIFIER
	{
		$$ = $1;
		$$->contents.push_back(make_pair(*$4, $3));
		delete $4;
	}
	| tuple_list ',' type
	{
		$$ = $1;
		$$->contents.push_back(make_pair("", $3));
	}
	;

type
	: primitive_type {$$ = $1;}
	| list_type {$$ = $1;}
	| tensor_type {$$ = $1;}
	| tuple_type {$$ = $1;}
	| '@' type
	{
		if (IS(RefType,$2))
		{
			yyerror("Cannot reference a reference");
			$$ = $2;
		}
		else
			$$ = new RefType($2);
	}
	| TYPE_NAME
	{
		$$ = typedefs[*$1]->clone();
		$$->name = *$1;
		delete $1;
	}
	| '*'
	{
		$$ = new AnyType();
	}
	;

/******************************************************** Declarations */

declaration
	: type var_name_list ';'
	{
		if ($1->abstract())
		{
			yyerror("Cannot instantiate abstract type " + $1->to_str());
		//	keep the symbol with the abstract type to give more informative errors later on
		}
		if (IS(RefType, $1))
			yyerror("References must be initialized");
		
		for (vector<VarDecl*>::iterator it = blocks_cur.back()->decls.begin();
			it != blocks_cur.back()->decls.end(); ++it)
			if (!(*it)->type)
				(*it)->type = new TypeWrapper($1);
			
	}
	| type IDENTIFIER '=' expression ';'
	{
		if (IS(RefType, $1))
			yyerror("Reference types must be initialized with @=");

		while (blocks > blocks_cur.size()) //we are the first one in the block
			blocks_cur.push_back(new Block(@1));
		
		VarDecl * d = new VarDecl(@2);
		d->type = new TypeWrapper($1);
		d->name = *$2;
		d->init = $4;
		blocks_cur.back()->decls.push_back(d);
		delete $2;
	}
	| type IDENTIFIER REF_ASSIGN expression ';'
	{
		if (!IS(RefType, $1))
			yyerror("Non-reference types must be initialized with =");

		while (blocks > blocks_cur.size()) //we are the first one in the block
			blocks_cur.push_back(new Block(@1));
		
		VarDecl * d = new VarDecl(@2);
		d->type = new TypeWrapper($1);
		d->name = *$2;
		d->init = $4;
		blocks_cur.back()->decls.push_back(d);
		delete $2;
	}
	;
	
var_name_list
	: IDENTIFIER
	{
		while (blocks > blocks_cur.size()) //we are the first one in the block
			blocks_cur.push_back(new Block(@1));
		
		VarDecl * d = new VarDecl(@1);
		d->name = *$1;
		delete $1;
		blocks_cur.back()->decls.push_back(d);
	}
	| var_name_list ',' IDENTIFIER
	{
		VarDecl * d = new VarDecl(@3);
		d->name = *$3;
		delete $3;
		blocks_cur.back()->decls.push_back(d);
	}
	;

type_declaration
	: TYPEDEF IDENTIFIER '=' type ';'
	{
		typedefs[*$2] = $4;
		delete $2;
	}
	| TYPEDEF TYPE_NAME '=' type ';'
	{
		yyerror("Redefinition of type " + *$2);
	}
	;

function_name
	: FUNC_NAME
	| IDENTIFIER
	;

func_ret
	: ':'
	{
		$$ = new AnyType();
	}
	| type ':'
	;

function_header
	: func_ret tuple_type function_name
	{
		for (TupleMap::iterator it = $2->contents.begin(); it != $2->contents.end(); ++it)
			it->second = new TypeWrapper(it->second); //wrap em up
		func_cur = new Func($2, new TypeWrapper($1), *$3, @3);
		funcdefs.push_back(*$3);
		delete $3;
	}
	| INLINE ':' func_ret  tuple_type function_name
	| CIMP ':' func_ret tuple_type function_name
	| CEXP ':' func_ret tuple_type function_name
	{
		for (TupleMap::iterator it = $4->contents.begin(); it != $4->contents.end(); ++it)
			it->second = new TypeWrapper(it->second); //wrap em up
		func_cur = new Func($4, new TypeWrapper($3), *$5, @5);
		func_cur->cexp = true;
		funcdefs.push_back(*$5);
		delete $5;
	}
	;

function_declaration
	: function_header ';'
	{
		if (!func_cur->argst->abstract())
		{
			if (Rejigger::DoFunction(func_cur)) //ignore function if error occurs
				file_cur.contents.push_back(func_cur);
		}
		else
			yyerror("Template functions haven't been implemented yet");
		func_cur = 0;
	}
	;

function_definition
	: function_header '=' compound_statement ';'
	{
		func_cur->conts = $3;
		if (!func_cur->argst->abstract())
		{
			file_cur.contents.push_back(func_cur);
			Rejigger::DoFunction(func_cur);
		}
		else
			yyerror("Template functions haven't been implemented yet");
		func_cur = 0;
	}
	;

global_declaration
	/*: declaration */
	: type_declaration
	| function_declaration
	| function_definition
	| error
	;

/******************************************************** Statements */

statement
	: compound_statement
	| expression_statement {$$ = $1;}
	| selection_statement
	| iteration_statement
	| jump_statement
//	| IDENTIFIER ':' statement
	| error 
	{
		$$ = new NullStmt(@1);
	}
	;

compound_statement
	: '{' '}'
	{
		$$ = new Block(@1);
	}
	| '{' block_item_list '}'
	{
		$$ = blocks_cur.back();
		blocks_cur.pop_back();
		$$->parent = blocks_cur.size() ? blocks_cur.back() : 0;
	}
	;

block_item_list
	: declaration
	{
	}
	| statement
	{
		blocks_cur.push_back(new Block(@1));
//		if ($1)
//		{
			blocks_cur.back()->stmts.push_back($1);
//		}
	}
	| block_item_list declaration
	{
	}
	| block_item_list statement
	{
//		if ($2)
//		{
			blocks_cur.back()->stmts.push_back($2);
//		}
	}
	;

expression_statement
	: ';'
	{
		$$ = new NullStmt(@1);
	}
	| expression ';'
	{
		$$ = new ExprStmt($1, @1);
	}
	;

selection_statement
	: SWITCH '(' expression ')' '{' labeled_statements '}'
	| IF '(' expression ')' statement %prec LOWER_THAN_ELSE
	{
		$$ = new IfStmt($3, $5, @1);
	}
	| IF '(' expression ')' statement ELSE statement
	{
		$$ = new IfStmt($3, $5, $7, @1);
	}
	;

labeled_statements
	: labeled_statement
	| labeled_statements labeled_statement
	;
	
labeled_statement
	: labeled_statement statement
	| CASE CONSTANT_INT ':' statement
	| DEFAULT ':' statement
	;

iteration_statement
	: WHILE '(' expression ')' statement
	{
		$$ = new WhileStmt($3, $5, @1);
	}
	| DO statement WHILE '(' expression ')' ';'
	{
	}
	for_header statement
	;
	
for_header /*create block so decl doesnt end up in outer block*/
	: FOR '(' expression_statement expression_statement ')'
	{
	}
	| FOR '(' expression_statement expression_statement expression ')'
	{
	}
	| FOR '(' declaration expression_statement ')'
	{
	}
	| FOR '(' declaration expression_statement expression ')'
	{
	}
	| FOR '(' IDENTIFIER ':' expression ')'
	;

jump_statement
	: CONTINUE ';'
	| BREAK ';'
//	| BREAK expression ';' //not yet
	| BREAK CONSTANT_INT ';'
	| RETURN ';'
	{
		$$ = new ReturnStmt(@1);
		if (!IS(AnyType, func_cur->rett->to))
			yyerror("Function must return a value");
	}
	| RETURN expression ';'
	{
		$$ = new ReturnValStmt($2, @1);
	}
	;

//	: GOTO IDENTIFIER ';'

/******************************************************** Whole file */

translation_unit
	: global_declarations
	{
		if (yyerrno > 0)
		{
			cerr << "Compliation failed with " << to_string(yyerrno) << " errors." << endl;
			return 1;
		}
		
		cout << header;
		
		const char * out = file_cur.output().c_str();
		int indent = 0;
		while (*out) //pretty print
		{
			putchar(*out);
			if (out[1] == '}')
				indent -= 4;
			switch (*out)
			{
				case '{': indent += 4;
				case ';': putchar('\n');
				case '\n':
					for (int i=0; i<indent; ++i)
						putchar(' ');
			}
			++out;
		}
	}
	;

global_declarations
	: global_declaration
	| global_declarations global_declaration
	;

%%

TypeWrapper * resolve(string name)
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

void yyerror(string s)
{
	yyerrno++;
	fflush(stdout);
	cerr << "Error in " << yylloc.filename << " at line " << yylloc.first_line
		<<" near '" << yytext << "': " << s << endl;
}

void yywarn(string s)
{
	fflush(stdout);
	cerr << "Warning in " << yylloc.filename << " at line " << yylloc.first_line
		<<" near '" << yytext << "': " << s << endl;
}

void yyerror(string s, YYLTYPE l)
{
	yyerrno++;
	fflush(stdout);
	cerr << "Error in " << l.filename << " at line " << l.first_line << ": " << s << endl;
}

void yywarn(string s, YYLTYPE l)
{
	fflush(stdout);
	cerr << "Warning in " << l.filename << " at line " << l.first_line << ": " << s << endl;
}
