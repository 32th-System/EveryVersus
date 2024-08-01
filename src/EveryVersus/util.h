#define elementsof(a) sizeof(a)/sizeof(a[0])
#define STRLEN_DEC(src_char) \
	size_t src_char##_len = strlen(src_char) + 1
