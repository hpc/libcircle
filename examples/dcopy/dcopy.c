#include <libcircle.h>
#include <log.h>
#include "dcopy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
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
    DIR *current_dir;
    char temp[CIRCLE_MAX_STRING_LEN];
    char stat_temp[CIRCLE_MAX_STRING_LEN];
    struct dirent *current_ent;
    struct stat st;
    char * dest_path;

    /* Clean up the destination input. */
    dest_path = dirname(DCOPY_DEST_PATH);

    /* Pop an item off the queue */
    handle->dequeue(temp);
    LOG(LOG_DBG, "Popped [%s]\n", temp);

    /* Try and stat it, checking to see if it is a link */
    if(lstat(temp,&st) != EXIT_SUCCESS)
    {
        LOG(LOG_ERR, "Error: Couldn't stat \"%s\"\n", temp);
        //MPI_Abort(MPI_COMM_WORLD,-1);
    }
    /* Check to see if it is a directory.  If so, put its children in the queue */
    else if(S_ISDIR(st.st_mode) && !(S_ISLNK(st.st_mode)))
    {
        current_dir = opendir(temp);

        if(!current_dir)
        {
            LOG(LOG_ERR, "Unable to open dir");
        }
        else
        {
            /* Since it's quick, just create the directory at the destination. */
            char * new_dir_name = malloc(snprintf(NULL, 0, "%s/%s", dest_path, temp) + 1);
            sprintf(new_dir_name, "%s/%s", dest_path, temp);
            LOG(LOG_DBG, "Creating directory with name: %s", new_dir_name);
            //mkdir(new_dir_name, st.st_mode);
            free(new_dir_name);

            /* Read in each directory entry */
            while((current_ent = readdir(current_dir)) != NULL)
            {
            /* We don't care about . or .. */
            if((strncmp(current_ent->d_name,".",2)) && (strncmp(current_ent->d_name,"..",3)))
                {
                    strcpy(stat_temp,temp);
                    strcat(stat_temp,"/");
                    strcat(stat_temp,current_ent->d_name);

                    LOG(LOG_DBG, "Pushing [%s] <- [%s]\n", stat_temp, temp);
                    handle->enqueue(&stat_temp[0]);
                }
            }
        }
        closedir(current_dir);
    }
    //else if(S_ISREG(st.st_mode) && (st.st_size % 4096 == 0))
    else if(S_ISREG(st.st_mode)) {
        LOG(LOG_DBG, "Copying: %s\n", temp);

        int infile;
        int outfile;

        if((infile = DCOPY_open_infile(temp)) < 0)
        {
            LOG(LOG_ERR, "Something went wrong while trying to read in a source file.");
        }
        else
        {
            char * new_file_name = malloc(snprintf(NULL, 0, "%s/%s", dest_path, temp) + 1);
            sprintf(new_file_name, "%s/%s", dest_path, temp);
            LOG(LOG_DBG, "Starting a copy to: %s", new_file_name);

            if((outfile = DCOPY_open_outfile(new_file_name, infile)) < 0)
            {
                LOG(LOG_ERR, "Something went wrong while trying to open an output file.");
                free(new_file_name);
            }
            else
            {
                /* Looks like we have valid in and out files. Let's do this. */
                //if(DCOPY_copy_data(infile, outfile) < 0)
                if(1)
                {
                    LOG(LOG_ERR, "Something went wrong while trying to copy: %s", new_file_name);
                }
                else
                {
                    LOG(LOG_DBG, "Copying \"%s\" was successful.", new_file_name);
                }

                free(new_file_name);
            }
        }
    }
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
