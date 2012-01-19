#include <stdlib.h>
#include <string.h>
#include "vsl.h"

void vsl_init_LaX(LaX * La, size_t sz, int num)
{
	posix_memalign(&(La->a), 16, sz * num);
	La->len = num;
}

LaX * vsl_LaX_assign_LnX(LaX * La, void * Ln, size_t sz, int num)
{
	free(La->a);
	posix_memalign(&(La->a), 16, sz * num);
	memcpy(La->a, Ln, sz * num);
	La->len = num;
	return La;
}
