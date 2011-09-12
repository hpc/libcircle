#ifndef VFPRINTSTATF_H
#define VFPRINTSTATF_H

#include <stddef.h>
#include <sys/stat.h>

int vfprintstatf(char *s, char *format, struct stat *stbuf);
char *lsmodes(int mode);
void lsrwx(char *p, int mode);

#endif /* VFPRINTSTATF_H */
