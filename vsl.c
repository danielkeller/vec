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

Ln * vsl_Ln_assign_Ln(Ln * La, Ln * Lb, size_t sz)
{
	int num = _vsl_Ln_size(Lb);
	free(La->a);
	posix_memalign(&(La->a), 16, sz * num);
	memcpy(La->a, Lb->a, sz * num);
	La->len = Lb->len;
	return La;
}

Ln * vsl_Ln_concat_Ln(Ln * La, Ln * Lb, size_t sz)
{
	//these lists will be 1 dimensional
	void * newmem;
	posix_memalign(&newmem, 16, sz * (La->len[0] + Lb->len[0]));
	memcpy(newmem, La->a, sz * La->len[0]);
	memcpy(newmem + sz * La->len[0], Lb->a, sz * Lb->len[0]);
	free(La->a);
	La->a = newmem;
	La->len[0] += Lb->len[0];
	return La;
}

/*
LaX * vsl_LaX_concat_LaX(LaX * Ll, LaX * Lr, size_t sz)
{
*/
