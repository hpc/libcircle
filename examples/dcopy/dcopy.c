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
DCOPY_add_objects(CIRCLE_handle *handle)
{
    handle->enqueue(DCOPY_SRC_PATH);
}

void
DCOPY_process_objects(CIRCLE_handle *handle)
{
    DIR *current_dir;
    char temp[CIRCLE_MAX_STRING_LEN];
    char stat_temp[CIRCLE_MAX_STRING_LEN];
    struct dirent *current_ent;
    struct stat st;

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
            char * new_dir_name = malloc(snprintf(NULL, 0, "%s/%s", DCOPY_DEST_PATH, temp) + 1);
            sprintf(new_dir_name, "%s/%s", DCOPY_DEST_PATH, temp);
            LOG(LOG_DBG, "Creating directory with name: %s", new_dir_name);
            mkdir(new_dir_name, st.st_mode);
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

        FILE *infile;
        FILE *outfile;

        if((infile = DCOPY_open_infile(temp)) == NULL)
        {
            LOG(LOG_ERR, "Something went wrong while trying to read in a source file.");
        }
        else
        {
            char *base_name = basename(temp);
            char *new_file_name = malloc(snprintf(NULL, 0, "%s/%s", DCOPY_DEST_PATH, base_name) + 1);
            sprintf(new_file_name, "%s/%s", DCOPY_DEST_PATH, base_name);

            LOG(LOG_DBG, "Dest path is: %s", DCOPY_DEST_PATH);
            LOG(LOG_DBG, "Dest file is: %s", base_name);
            LOG(LOG_DBG, "Starting a copy to: %s", new_file_name);

            if((outfile = DCOPY_open_outfile(new_file_name, infile)) == NULL)
            {
                LOG(LOG_ERR, "Something went wrong while trying to open an output file.");
            }
            else
            {
                /* Looks like we have valid in and out files. Let's do this. */
                if(DCOPY_copy_data(infile, outfile) < 0)
                {
                    LOG(LOG_ERR, "Something went wrong while trying to copy: %s", new_file_name);
                    
                }
                else
                {
                    LOG(LOG_DBG, "Copying \"%s\" was successful.", new_file_name);
                }
            }

            fclose(infile);
            fclose(outfile);
            free(new_file_name);
        }
    }
}

int
DCOPY_copy_data(FILE *fin, FILE *fout)
{
    char    buffer[DCOPY_FILECOPY_BUFFER_SIZE];
    size_t  n;

    while ((n = fread(buffer, sizeof(char), sizeof(buffer), fin)) > 0)
    {
        if (fwrite(buffer, sizeof(char), n, fout) != n)
        {
            LOG(LOG_FATAL, "Writing a file failed.");
            return -1;
        }
    }

    return 0;
}

FILE *
DCOPY_open_infile(char *infile)
{
    FILE *fin = fopen(infile, "rb");

    if (fin == NULL) {
        LOG(LOG_ERR, "Could not open the source file: %s", infile);
        return NULL;
    }

    return fin;
}

FILE *
DCOPY_open_outfile(char *outfile, FILE *fin)
{
    FILE *fout = fopen(outfile, "wb");

    if (fout == NULL)
    {
        LOG(LOG_ERR, "Could not open the destination file: %s", outfile);
        fclose(fin);
        return NULL;
    }

    return fout;
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
                DCOPY_DEST_PATH = realpath(optarg, NULL);
                break;
            case 's':
                DCOPY_SRC_PATH = realpath(optarg, NULL);
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

    CIRCLE_cb_create (&DCOPY_add_objects);
    CIRCLE_cb_process(&DCOPY_process_objects);

    CIRCLE_begin();
    CIRCLE_finalize();

    free(DCOPY_DEST_PATH);
    free(DCOPY_SRC_PATH);

    exit(EXIT_SUCCESS);
}

/* EOF */
