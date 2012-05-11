typedef struct {
	int dim;
	int * len;
	void * a;
} Ln;

void vsl_init_Ln(Ln * La, size_t sz, int dim, ...);
Ln * vsl_Ln_assign_Ln(Ln * La, Ln * Lb, size_t sz);
Ln * vsl_Ln_concat_Ln(Ln * La, Ln * Lb, size_t sz);
