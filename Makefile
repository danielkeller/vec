.KEEP_STATE:
SHELL=/bin/bash

#
#	define version of cpp compiler, linker and lex
#
CC=		g++
LINK=	g++
LEX=	flex
#
#	define yacc lex and compiler flags
#
YFLAGS	= -dv
LFLAGS	=
CFLAGS	= -g -rdynamic
CPPFLAGS	= -g -rdynamic

OBJ	= gram.o scan.o main.o   type.o   stmt.o   tables.o   rejigger.o

vec :	$(OBJ)
	$(LINK) $(CFLAGS) $(OBJ) -o vec

$(OBJ) : *.h

clean	: 
	rm -f y.tab.h y.output *.o

rem : clean vec

#vsl.o :
#	gcc -c vsl.c

test : vec test.vc vsl.c
	./v $@.vc > $@.c
	gcc -g $@.c vsl.c -o $@
