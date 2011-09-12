#ifndef SPRINTSTATF_H
#define SPRINTSTATF_H

#include <stddef.h>
#include <sys/stat.h>

int sprintstatf(char *out, char *format, struct stat *stbuf);
char *lsmodes(int mode);
void lsrwx(char *p, int mode);

#endif /* SPRINTSTATF_H */
