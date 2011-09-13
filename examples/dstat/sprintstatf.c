#include "sprintstatf.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <grp.h>
#include <pwd.h>

/*
 * See <http://github.com/hpc/sprintstatf> for more information on this.
 */

int
sprintstatf(char *out, char *format, struct stat *stbuf)
{
    char buf[12];
    char *p;
    struct passwd *pwp;
    struct group *grp;
    int i = 0;

    if(format != NULL && stbuf != NULL)
    {
        for(p = format; *p != '\0'; p++)
        {
            if(*p != '%')
            {
                out[i++] = *p;
                continue;
            }

            switch(*++p)
            {
                case 'a':
                     i += sprintf(out+i, "%ld", (long)stbuf->st_atime);
                    break;

                case 'A':
                    i += sprintf(out+i, "%.24s", ctime(&(stbuf->st_atime)));
                    break;

                case 'c':
                    i += sprintf(out+i, "%ld", (long)stbuf->st_ctime);
                    break;

                case 'C':
                    i += sprintf(out+i, "%.24s", ctime(&(stbuf->st_ctime)));
                    break;

                case 'g':
                    i += sprintf(out+i, "%d", stbuf->st_gid);
                    break;

                case 'G':
                    grp = getgrgid(stbuf->st_gid);
                    i += sprintf(out+i, "%s",
                        (grp != NULL) ? grp->gr_name :
                            "");
                    break;

                case 'i':
                    i += sprintf(out+i, "%llu", (long long)stbuf->st_ino);
                    break;

                case 'm':
                    i += sprintf(out+i, "%ld", (long)stbuf->st_mtime);
                    break;

                case 'M':
                    i += sprintf(out+i, "%.24s", ctime(&(stbuf->st_mtime)));
                    break;

                case 'n':
                    i += sprintf(out+i, "%d", (unsigned int)stbuf->st_nlink);
                    break;

                case 'p':
                    i += sprintf(out+i, "%o", stbuf->st_mode);
                    break;

                case 'P':
                    lsmodes(buf,stbuf->st_mode);
                    i += sprintf(out+i, "%s", buf);
                    break;

                case 's':
                    i += sprintf(out+i, "%llu", (long long)stbuf->st_size);
                    break;

                case 'u':
                    i += sprintf(out+i, "%d", stbuf->st_uid);
                    break;

                case 'U':
                    pwp = getpwuid(stbuf->st_uid);
                    i += sprintf(out+i, "%s",
                        (pwp != NULL) ? pwp->pw_name :
                            "");
                    break;

                case '%':
                    out[i++] = '%';
                    break;

                /* default ignored */
            }
        }
        out[i] = '\0';
    }

    return i;
}

void
lsmodes(char * retbuf, int mode)
{
    int ifmt = mode & S_IFMT;

    if(ifmt == S_IFDIR)
        retbuf[0] = 'd';
    else if(ifmt == S_IFCHR)
        retbuf[0] = 'c';
    else if(ifmt == S_IFBLK)
        retbuf[0] = 'b';
    else if(ifmt == S_IFLNK)
        retbuf[0] = 'l';
    else if(ifmt == S_IFSOCK)
        retbuf[0] = 's';
    else if(ifmt == S_IFIFO)
        retbuf[0] = 'p';
    else
        retbuf[0] = '-';

    lsrwx(&retbuf[1], mode);
    lsrwx(&retbuf[4], mode << 3);
    lsrwx(&retbuf[7], mode << 6);

    if(mode & S_ISUID)
        retbuf[3] = (mode & S_IEXEC) ? 's' : 'S';

    if(mode & S_ISGID)
        retbuf[6] = (mode & (S_IEXEC >> 3)) ? 's' : 'S';

    if(mode & S_ISVTX)
        retbuf[9] = (mode & (S_IEXEC >> 6)) ? 't' : 'T';

    retbuf[10] = '\0';

}

void
lsrwx(char *p, int mode)
{
    if(mode & S_IREAD)
        p[0] = 'r';
    else
        p[0] = '-';

    if(mode & S_IWRITE)
        p[1] = 'w';
    else
        p[1] = '-';

    if(mode & S_IEXEC)
        p[2] = 'x';
    else
        p[2] = '-';
}

/* EOF */
