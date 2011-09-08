#include <libcircle.h>
#include "dcopy.h"
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>

char *DCOPY_SRC_PATH;
char *DCOPY_DEST_PATH;

void
dcopy_add_objects(CIRCLE_handle *handle)
{
    handle->enqueue(DCOPY_SRC_PATH);
}

void
dcopy_process_objects(CIRCLE_handle *handle)
{
    /* TODO */
}

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
DCOPY_open_infile(char *infile)
{
    int fdin = open(infile, O_RDONLY, 0);

    if (fdin == -1) {
        return -1;
    }

    return fdin;
}

int
DCOPY_open_outfile(char *outfile, int fdin)
{
    int fdout = open(outfile, O_WRONLY|O_CREAT|O_TRUNC, 0x1ff);

    if (fdout == -1)
    {
        close(fdin);
        return -1;
    }

    return fdout;
}

int
main (int argc, char **argv)
{
    int index;
    int c;
     
    opterr = 0;
    while((c = getopt(argc, argv, "s:d:")) != -1)
    {
        switch(c)
        {
            case 'd':
                DCOPY_DEST_PATH = optarg;
                break;
            case 's':
                DCOPY_SRC_PATH = optarg;
                break;
            case '?':
                if (optopt == 'd' || optopt == 's')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);

                exit(EXIT_FAILURE);
            default:
                abort();
        }
    }

    for (index = optind; index < argc; index++)
        printf ("Non-option argument %s\n", argv[index]);

    CIRCLE_init(argc, argv);
    CIRCLE_cb_create(&dcopy_add_objects);
    CIRCLE_cb_process(&dcopy_process_objects);
    CIRCLE_begin();
    CIRCLE_finalize();

    exit(EXIT_SUCCESS);
}

/* EOF */
