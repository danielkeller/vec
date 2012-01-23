typedef struct {
	int len;
	void * a;
} LaX;
typedef LaX LnX;

void vsl_init_LaX(LaX * La, size_t sz, int num);
void vsl_init_LnX(LaX * La, size_t sz, int num);
LaX * vsl_LaX_assign_LnX(LaX * La, LnX * Ln, size_t sz, int num);
