#include "yy.h"
#include "string.h"
#include <execinfo.h>
#include <signal.h>

void handler(int sig) {
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, 2);
    fflush(stderr);
    exit(1);
}

int main(int argc, char*argv[])
{
	signal(SIGSEGV, handler);
	signal(SIGFPE, handler);

	if(argc > 1)
		if (strncmp(argv[1], "-d", 2)==0)
			yydebug = 1;
	
	srand ( time(NULL) );
	
	return yyparse();
}
