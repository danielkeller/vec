%option yylineno
%{
//%option stack
//%option bison-bridge
//%option bison-locations
//%x COMMENT
//"/*"			{ yy_push_state( COMMENT); }
//<COMMENT>"*/"	{ yy_pop_state( ); }
//<COMMENT>.		{}
//"//"[^\n]*      { // consume //-comment  }
//IS          ((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))


#include "yy.h"
void count(void);
int check_type(void);
#define YY_USER_ACTION yylloc.first_line = yylineno; yylloc.filename = fname;
char * fname = 0;
%}

D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
P         		[Pp][+-]?{D}+
FS			(f|F|l|L)
IS         		(l|L|s|S|c|C)


%%
"break"			{ return(BREAK); }
"case"			{ return(CASE); }
"continue"		{ return(CONTINUE); }
"default"		{ return(DEFAULT); }
"do"			{ return(DO); }
"else"			{ return(ELSE); }
"float"			{ return(FLOAT); }
"for"			{ return(FOR); }
"while"			{ return(WHILE); }
"goto"			{ return(GOTO); }
"if"			{ return(IF); }
"int"			{ return(INT); }
"return"		{ return(RETURN); }
"switch"		{ return(SWITCH); }
"type"			{ return(TYPEDEF); }
"inline"		{ return(INLINE); }
"cimp"			{ return(CIMP); }
"cexp"			{ return(CEXP); }
"#".+\n			{
					char * fn = strchr(yytext+2, ' ');
					*fn = 0;
					strchr(fn+2, '"')[1] = 0;
					fname = strdup(fn+1);
					yylineno = atoi(yytext+2);
				}

{L}({L}|{D})*		{ yylval.txt_v = new string(yytext); return check_type();}

0[xX]{H}+{IS}?		{ yylval.long_v = strtol(yytext, 0, 0); return(CONSTANT_INT); }
{D}+{IS}?		{ yylval.long_v = atol(yytext); return(CONSTANT_INT); }
L?'(\\.|[^\\'\n])+'	{ return(CONSTANT_INT); }

{D}+{E}{FS}?		{ yylval.txt_v = new string(yytext); return(CONSTANT_FLOAT); }
{D}*"."{D}+({E})?{FS}?	{ yylval.txt_v = new string(yytext); return(CONSTANT_FLOAT); }
{D}+"."{D}*({E})?{FS}?	{ yylval.txt_v = new string(yytext); return(CONSTANT_FLOAT); }


L?\"(\\.|[^\\"\n])*\"	{ yytext[strlen(yytext)-1] = 0; yylval.txt_v = new string(yytext+1); return(STRING_LITERAL); }

">>="			{ return(RIGHT_ASSIGN); }
"<<="			{ return(LEFT_ASSIGN); }
"+="			{ return(ADD_ASSIGN); }
"-="			{ return(SUB_ASSIGN); }
"*="			{ return(MUL_ASSIGN); }
"/="			{ return(DIV_ASSIGN); }
"%="			{ return(MOD_ASSIGN); }
"&="			{ return(AND_ASSIGN); }
"^="			{ return(XOR_ASSIGN); }
"|="			{ return(OR_ASSIGN); }
"@="			{ return(REF_ASSIGN); }
">>"			{ return(RIGHT_OP); }
"<<"			{ return(LEFT_OP); }
"++"			{ return(INC_OP); }
"--"			{ return(DEC_OP); }
"&&"			{ return(AND_OP); }
"||"			{ return(OR_OP); }
"<="			{ return(LE_OP); }
">="			{ return(GE_OP); }
"=="			{ return(EQ_OP); }
"!="			{ return(NE_OP); }
";"			{ return(';'); }
"{"			{ blocks++; return('{'); }
"}"			{ blocks--; return('}'); }
","			{ return(','); }
":"			{ return(':'); }
"="			{ return('='); }
"("			{ return('('); }
")"			{ return(')'); }
"["			{ return('['); }
"]"			{ return(']'); }
"."			{ return('.'); }
"!"			{ return('!'); }
"~"			{ return('~'); }
"-"			{ return('-'); }
"+"			{ return('+'); }
"*"			{ return('*'); }
"&"			{ return('&'); }
"@"			{ return('@'); }
"/"			{ return('/'); }
"%"			{ return('%'); }
"<"			{ return('<'); }
">"			{ return('>'); }
"^"			{ return('^'); }
"|"			{ return('|'); }
"?"			{ return('?'); }
"$"			{ return('$'); }

[ \t\v\n\f]		{ }
.			{ yyerror("Invalid character"); }

%%

/*
0[xX]{H}+{P}{FS}?               { return(CONSTANT); }
0[xX]{H}*"."{H}+({P})?{FS}?     { return(CONSTANT); }
0[xX]{H}+"."{H}*({P})?{FS}?     { return(CONSTANT); }
*/

int check_type(void)
{
	if (typedefs.count(yytext))
		return TYPE_NAME;
//	if (find(funcdefs.begin(), funcdefs.end(), yytext) != funcdefs.end())
//		return FUNC_NAME;

	return IDENTIFIER;
}

int yywrap(void)
{
	return 1;
}
