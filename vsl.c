#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "vsl.h"

int _vsl_Ln_size(Ln * l)
{
	int i, sz = 1;
	for (i=0; i < l->dim; ++i)
		sz *= l->len[i];
	return sz;
}

void vsl_init_Ln(Ln * La, size_t sz, int dim, ...)
{
	va_list vl;
	int i, num = 1;
	va_start(vl, dim);
	La->dim = dim;
	La->len = malloc(dim);
	for (i=0; i<dim; ++i)
		num *= (La->len[i] = va_arg(vl, int));
	posix_memalign(&(La->a), 16, sz * num);
}

Ln * vsl_Ln_assign_Ln(Ln * La, Ln * Ln, size_t sz)
{
	int num = _vsl_Ln_size(Ln);
	free(La->a);
	posix_memalign(&(La->a), 16, sz * num);
	memcpy(La->a, Ln->a, sz * num);
	La->len = Ln->len;
	return La;
}
/*
LaX * vsl_LaX_concat_LaX(LaX * Ll, LaX * Lr, size_t sz)
{
*/
