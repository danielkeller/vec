#include <stdlib.h>
#include <string.h>
#include "vsl.h"

void vsl_init_LaX(LaX * La, size_t sz, int num)
{
	posix_memalign(&(La->a), 16, 0);
	La->len = 0;
}

void vsl_init_LnX(LaX * La, size_t sz, int num)
{
	posix_memalign(&(La->a), 16, sz * num);
	La->len = num;
}

LaX * vsl_LaX_assign_LnX(LaX * La, LnX * Ln, size_t sz, int num)
{
	free(La->a);
	posix_memalign(&(La->a), 16, sz * num);
	memcpy(La->a, Ln->a, sz * num);
	La->len = num;
	return La;
}
/*
LaX * vsl_LaX_concat_LaX(LaX * Ll, LaX * Lr, size_t sz)
{
*/
