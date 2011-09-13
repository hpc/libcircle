#ifndef SPRINTSTATF_H
#define SPRINTSTATF_H

/*
 * See <http://github.com/hpc/sprintstatf> for more information on this.
 */

#include <stddef.h>
#include <sys/stat.h>

int sprintstatf(char *out, char *format, struct stat *stbuf);
void lsmodes(char * retbuf,int mode);
void lsrwx(char *p, int mode);

#endif /* SPRINTSTATF_H */
