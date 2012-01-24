#include <stdlib.h>
#include <string.h>
#include "vsl.h"

void vsl_init_LaX(LaX * La, size_t sz, int num)
{
	posix_memalign(&(La->a), 16, sz * num);
	La->len = num;
}

void vsl_init_LnX(LaX * La, size_t sz, int num)
{
	posix_memalign(&(La->a), 16, sz * num);
	La->len = num;
}

LaX * vsl_LaX_assign_LaX(LaX * La, LaX * Ln, size_t sz)
{
	free(La->a);
	posix_memalign(&(La->a), 16, sz * Ln->len);
	memcpy(La->a, Ln->a, sz * Ln->len);
	La->len = Ln->len;
	return La;
}
/*
LaX * vsl_LaX_concat_LaX(LaX * Ll, LaX * Lr, size_t sz)
{
*/
