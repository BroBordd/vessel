#include <stddef.h>
#include <errno.h>
void *libiconv_open(const char *t, const char *f) { errno = EINVAL; return (void *)-1; }
int libiconv_close(void *cd) { return 0; }
size_t libiconv(void *cd, char **i, size_t *il, char **o, size_t *ol) { errno = EILSEQ; return (size_t)-1; }
