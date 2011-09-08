#include <libcircle.h>
#include "dcopy.h"
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>

void
DCOPY_block(int fd, int event)
{
    struct pollfd topoll;
    topoll.fd = fd;
    topoll.events = event;
    poll(&topoll, 1, -1);
}

int
DCOPY_copy_data_buffer(int fdin, int fdout, void *buf, size_t bufsize)
{
    for(;;)
    {
       void *pos;
       ssize_t bytestowrite = read(fdin, buf, bufsize);

       if (bytestowrite == 0)
       {
           /* End of input */
           break;
       }

       if (bytestowrite == -1)
       {
           if (errno == EINTR)
           {
               /* Signal handled */
               continue;
           }

           if (errno == EAGAIN)
           {
               DCOPY_block(fdin, POLLIN);
               continue;
           }

           /* Error */
           return -1;
       }

       /* Write data from buffer */
       pos = buf;
       while (bytestowrite > 0)
       {
           ssize_t bytes_written = write(fdout, pos, bytestowrite);

           if (bytes_written == -1)
           {
               if (errno == EINTR)
               {
                   /* Signal handled */
                   continue;
               }

               if (errno == EAGAIN)
               {
                   DCOPY_block(fdout, POLLOUT);
                   continue;
               }

               /* Error */
               return -1;
           }

           bytestowrite -= bytes_written;
           pos += bytes_written;
       }
    }

    /* Success */
    return 0;
}

int
DCOPY_copy_data(int fdin, int fdout)
{
    /*
     * TODO: take the file size as a parameter,
     * and don't use a buffer any bigger than that. This prevents 
     * memory-hogging if DCOPY_FILECOPY_BUFFER_SIZE is very large and the file
     * is small.
     */

    size_t bufsize;
    for (bufsize = DCOPY_FILECOPY_BUFFER_SIZE; bufsize >= 256; bufsize /= 2)
    {
        void *buffer = malloc(bufsize);

        if (buffer != NULL)
        {
            int result = DCOPY_copy_data_buffer(fdin, fdout, buffer, bufsize);
            free(buffer);
            return result;
        }
    }

    /*
     * could use a stack buffer here instead of failing, if desired.
     * 128 bytes ought to fit on any stack worth having, but again
     * this could be made configurable.
     */

    /* Error is ENOMEM */
    return -1; // errno is ENOMEM
}

int
main (void)
{
    /* TODO */
    return 0;
}

/* EOF */
