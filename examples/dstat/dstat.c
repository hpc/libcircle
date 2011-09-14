#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <libcircle.h>
#include <log.h>
#include "sprintstatf.h"
#include "hash.h"

#include <hiredis.h>
#include <async.h>

char *TOP_DIR;
redisAsyncContext *REDIS;

void
add_objects(CIRCLE_handle *handle)
{
    handle->enqueue(TOP_DIR);
}

void
process_objects(CIRCLE_handle *handle)
{
    DIR *current_dir;
    char temp[CIRCLE_MAX_STRING_LEN];
    char stat_temp[CIRCLE_MAX_STRING_LEN];
    struct dirent *current_ent; 
    struct stat st;

    int hash_idx = 0;
    unsigned char filename_hash[32];

    int  redis_cmd_idx = 0;
    char *redis_cmd_buf = (char *)malloc(2048 * sizeof(char));
    char *redis_cmd_fmt = (char *)malloc(2048 * sizeof(char));
    char *redis_cmd_fmt_cnt = \
            "atime_decimal \"%a\" "
            "atime_string  \"%A\" "
            "ctime_decimal \"%c\" "
            "ctime_string  \"%C\" "
            "gid_decimal   \"%g\" "
            "gid_string    \"%G\" "
            "ino           \"%i\" "
            "mtime_decimal \"%m\" "
            "mtime_string  \"%M\" "
            "nlink         \"%n\" "
            "mode_octal    \"%p\" "
            "mode_string   \"%P\" "
            "size          \"%s\" "
            "uid_decimal   \"%u\" "
            "uid_string    \"%U\" ";

    /* Pop an item off the queue */ 
    handle->dequeue(temp);
    LOG(LOG_DBG, "Popped [%s]", temp);

    /* Try and stat it, checking to see if it is a link */
    if(lstat(temp,&st) != EXIT_SUCCESS)
    {
            LOG(LOG_ERR, "Error: Couldn't stat \"%s\"", temp);
            //MPI_Abort(MPI_COMM_WORLD,-1);
    }
    /* Check to see if it is a directory.  If so, put its children in the queue */
    else if(S_ISDIR(st.st_mode) && !(S_ISLNK(st.st_mode)))
    {
        current_dir = opendir(temp);
        if(!current_dir) {
            LOG(LOG_ERR, "Unable to open dir");
        }
        else
        {
            /* Read in each directory entry */
            while((current_ent = readdir(current_dir)) != NULL)
            {
            /* We don't care about . or .. */
            if((strncmp(current_ent->d_name,".",2)) && (strncmp(current_ent->d_name,"..",3)))
                {
                    strcpy(stat_temp,temp);
                    strcat(stat_temp,"/");
                    strcat(stat_temp,current_ent->d_name);

                    LOG(LOG_DBG, "Pushing [%s] <- [%s]", stat_temp, temp);
                    handle->enqueue(&stat_temp[0]);
                }
            }
        }
        closedir(current_dir);
    }
    else if(S_ISREG(st.st_mode)) {
        /* Add the header */
        redis_cmd_idx = sprintf(redis_cmd_fmt, "HMSET id:");

        /* Generate and add the key */
        dstat_filename_hash(filename_hash, (unsigned char *)temp);
        for(hash_idx = 0; hash_idx < 32; hash_idx++)
            redis_cmd_idx += sprintf(redis_cmd_fmt + redis_cmd_idx, "%02x", filename_hash[hash_idx]);

        /* Add the filename itself to the redis set command */
        redis_cmd_idx += sprintf(redis_cmd_fmt + redis_cmd_idx, " name \"%s\"",temp);

        /* Add the args for sprintstatf */
        sprintf(redis_cmd_fmt + redis_cmd_idx, " %s", redis_cmd_fmt_cnt);

        redis_cmd_idx += sprintstatf(redis_cmd_buf, redis_cmd_fmt, &st);
        LOG(LOG_DBG, "RedisCmd = \"%s\" Count = %d", redis_cmd_buf,redis_cmd_idx);

        if(redisAsyncCommand(REDIS, NULL, NULL, redis_cmd_buf) == REDIS_OK)
        {
            LOG(LOG_DBG, "Sent %s to redis", temp);
        }
        else
        {
            LOG(LOG_DBG, "Failed to SET %s", temp);
            if (REDIS->err)
            {
                LOG(LOG_ERR, "Redis error: %s", REDIS->errstr);
            }
        }
    }

    free(redis_cmd_buf);
    free(redis_cmd_fmt);
}

void
print_usage(char **argv)
{
    fprintf(stderr, "Usage: %s -d <starting directory> [-h <redis_hostname> -p <redis_port>]\n", argv[0]);
}

int
main (int argc, char **argv)
{
    int index;
    int c;

    char *redis_hostname;
    int redis_port;

    int dir_flag = 0;
    int redis_hostname_flag = 0;
    int redis_port_flag = 0;
     
    opterr = 0;
    while((c = getopt(argc, argv, "d:h:p:")) != -1)
    {
        switch(c)
        {
            case 'd':
                TOP_DIR = optarg;
                dir_flag = 1;
                break;

            case 'h':
                redis_hostname = optarg;
                redis_hostname_flag = 1;
                break;

            case 'p':
                redis_port = atoi(optarg);
                redis_port_flag = 1;
                break;

            case '?':
                if (optopt == 'd' || optopt == 'h' || optopt == 'p')
                {
                    print_usage(argv);
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                    exit(EXIT_FAILURE);
                }
                else if (isprint (optopt))
                {
                    print_usage(argv);
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                    exit(EXIT_FAILURE);
                }
                else
                {
                    print_usage(argv);
                    fprintf(stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
                    exit(EXIT_FAILURE);
                }

            default:
                abort();
        }
    }

    if(dir_flag == 0)
    {
         print_usage(argv);
         LOG(LOG_FATAL, "You must specify a starting directory");
         exit(EXIT_FAILURE);
    }

    if(redis_hostname_flag == 0)
    {
        LOG(LOG_WARN, "A hostname for redis was not specified, defaulting to localhost.");
        redis_hostname = "localhost";
    }

    if(redis_port_flag == 0)
    {
        LOG(LOG_WARN, "A port number for redis was not specified, defaulting to 6379.");
        redis_port = 6379;
    }

    for (index = optind; index < argc; index++)
        LOG(LOG_WARN, "Non-option argument %s", argv[index]);

    REDIS = redisAsyncConnect(redis_hostname, redis_port);
    if (REDIS->err)
    {
        LOG(LOG_FATAL, "Redis error: %s", REDIS->errstr);
        exit(EXIT_FAILURE);
    }

    CIRCLE_init(argc, argv);

    CIRCLE_cb_create(&add_objects);
    CIRCLE_cb_process(&process_objects);

    CIRCLE_begin();
    CIRCLE_finalize();

    exit(EXIT_SUCCESS);
}

/* EOF */
