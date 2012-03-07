/**
 * @file
 * This file contains functions related to the local queue structure.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "libcircle.h"
#include "queue.h"
#include "log.h"

extern int8_t CIRCLE_ABORT_FLAG;

/**
 * Allocate memory for the basic queue structure used by libcircle.
 *
 * @return a reference to the allocated queue structure.
 *
 * @see CIRCLE_internal_queue_free
 */
CIRCLE_internal_queue_t* CIRCLE_internal_queue_init(void)
{
    CIRCLE_internal_queue_t* qp;

    LOG(CIRCLE_LOG_DBG, "Allocating a queue structure.");

    qp = (CIRCLE_internal_queue_t*) malloc(sizeof(CIRCLE_internal_queue_t));

    /* Number of string pointers we have allocated */
    qp->str_count = CIRCLE_INITIAL_INTERNAL_QUEUE_SIZE;

    /* Base address of string pool */
    qp->base = (char*) malloc(sizeof(char) * \
                              CIRCLE_MAX_STRING_LEN * \
                              qp->str_count);
    qp->count = 0;
    qp->head = 0;
    qp->end = qp->base + \
              (CIRCLE_MAX_STRING_LEN * qp->str_count);

    /* String pointer array */
    qp->strings = (uintptr_t*) malloc(sizeof(uintptr_t) * \
                                      qp->str_count);

    if(!qp || !qp->base || !qp->strings) {
        LOG(CIRCLE_LOG_ERR, "Failed to allocate a basic queue structure.");
        return (CIRCLE_internal_queue_t*) NULL;
    }

    return qp;
}

/**
 * Free the memory used by a libcircle basic queue structure.
 *
 * @param qp the reference to the queue that should be freed.
 * @return a negative value on failure, a positive one on success.
 */
int8_t CIRCLE_internal_queue_free(CIRCLE_internal_queue_t* qp)
{
    if(qp) {
        if(qp->strings) {
            LOG(CIRCLE_LOG_DBG, "Freeing the queue strings array.");
            free(qp->strings);
        }

        LOG(CIRCLE_LOG_DBG, "Freeing a queue pointer.");
        free(qp);
    }
    else {
        LOG(CIRCLE_LOG_ERR, "Attempted to free a null queue structure.");
        return -1;
    }

    return 1;
}

/**
 * Dump the raw contents of the queue structure to logging.
 *
 * @param qp the queue structure that should be dumped.
 */
void CIRCLE_internal_queue_dump(CIRCLE_internal_queue_t* qp)
{
    uint32_t i = 0;
    char* p = qp->base;

    while(p++ != (qp->base + qp->strings[qp->count - 1] + \
                  strlen(qp->base + qp->strings[qp->count - 1 ]))) {
        if(i++ % 120 == 0) {
            LOG(CIRCLE_LOG_DBG, "%c", *p);
        }
        else {
            LOG(CIRCLE_LOG_DBG, "%c", *p);
        }
    }
}

/**
 * Pretty-print the queue data structure.
 *
 * @param qp the queue structure that should be pretty-printed.
 */
void CIRCLE_internal_queue_print(CIRCLE_internal_queue_t* qp)
{
    uint32_t i = 0;

    for(i = 0; i < qp->count; i++) {
        LOG(CIRCLE_LOG_DBG, "\t[%p][%d] %s", \
            qp->base + qp->strings[i], i, qp->base + qp->strings[i]);
    }
}
/**
 * Extend the string array size size
 *
 */
int8_t CIRCLE_internal_queue_str_extend(CIRCLE_internal_queue_t* qp, \
                                        int new_size)
{
    int rc = qp->str_count;

    while((signed)qp->str_count < new_size) {
        qp->str_count += 4096;
    }

    size_t size = qp->str_count * sizeof(uintptr_t);
    qp->strings = (uintptr_t*) realloc(qp->strings, size);

    LOG(CIRCLE_LOG_DBG, "Reallocing string array from" \
        " [%d] to [%d] [%p] -> [%p]", rc, qp->str_count, \
        (void*)qp->strings, (void*)(qp->strings + size));

    if(!qp->strings) {
        LOG(CIRCLE_LOG_ERR, "Unable to realloc string array.");
        return -1;
    }

    return 0;
}

/**
 * Extend the circle queue size
 *
 */
int8_t CIRCLE_internal_queue_extend(CIRCLE_internal_queue_t* qp)
{
    size_t current = qp->end - qp->base;
    current += sysconf(_SC_PAGESIZE) * 4096;

    LOG(CIRCLE_LOG_DBG, "Reallocing queue from [%zd] to [%zd] [%p] -> [%p].", \
        (qp->end - qp->base), current, qp->base, qp->base + current);

    qp->base = (char*) realloc(qp->base, current);

    if(!qp->base) {
        LOG(CIRCLE_LOG_ERR, "Failed to reallocate a basic queue structure.");
        return -1;
    }

    qp->end = qp->base + current;
    return 0;
}

/**
 * Push the specified string onto the queue structure.
 *
 * @param qp the queue structure to push the value onto.
 * @param str the string value to push onto the queue.
 *
 * @return a positive number on success, a negative one on failure.
 */
int8_t CIRCLE_internal_queue_push(CIRCLE_internal_queue_t* qp, char* str)
{
    if(!str) {
        LOG(CIRCLE_LOG_ERR, "Attempted to push null pointer.");
        return -1;
    }

    uint32_t len = strlen(str);

    if(len <= 0) {
        LOG(CIRCLE_LOG_ERR, "Attempted to push an empty string onto a queue.");
        return -1;
    }

    if(qp->count > qp->str_count) {
        LOG(CIRCLE_LOG_DBG, "Extending string array by 4096.");

        if(CIRCLE_internal_queue_str_extend(qp, qp->count + 4096) < 0) {
            return -1;
        }
    }

    if(qp->count > 0) {
        if(qp->base + qp->strings[qp->count - 1] + \
                CIRCLE_MAX_STRING_LEN >= qp->end) {
            LOG(CIRCLE_LOG_DBG, \
                "The queue is not large enough to add another value.");

            if(CIRCLE_internal_queue_extend(qp) < 0) {
                return -1;
            }
        }
    }

    if(len > CIRCLE_MAX_STRING_LEN) {
        LOG(CIRCLE_LOG_ERR, \
            "Attempted to push a value that was larger than expected.");
        return -1;
    }

    /* Set our write location to the end of the current strings array. */
    qp->strings[qp->count] = qp->head;

    /* Copy the string. */
    strcpy(qp->base + qp->head, str);

    /*
     * Make head point to the character after the string (strlen doesn't
     * include a trailing null).
     */
    qp->head = qp->head + strlen(qp->base + qp->head) + 1;

    /* Make the head point to the next available memory */
    qp->count++;

    return 0;
}

/**
 * Removes an item from the queue and returns a copy.
 *
 * @param qp the queue structure to remove the item from.
 * @param str a reference to the value removed.
 *
 * @return a positive value on success, a negative one otherwise.
 */
int8_t CIRCLE_internal_queue_pop(CIRCLE_internal_queue_t* qp, char* str)
{
    if(!qp) {
        LOG(CIRCLE_LOG_ERR, "Attempted to pop from an invalid queue.");
        return -1;
    }

    if(qp->count < 1) {
        LOG(CIRCLE_LOG_DBG, "Attempted to pop from an empty queue.");
        return -1;
    }

    if(!str) {
        LOG(CIRCLE_LOG_ERR, \
            "You must allocate a buffer for storing the result.");
        return -1;
    }

    /* Copy last element into str */
    strcpy(str, qp->base + qp->strings[qp->count - 1]);
    qp->count = qp->count - 1;

    return 0;
}

/**
 * Read a queue checkpoint file into working memory.
 *
 * @param qp the queue structure to read the checkpoint file into.
 * @param rank the node which holds the checkpoint file.
 *
 * @return a positive value on success, a negative one otherwise.
 */
int8_t CIRCLE_internal_queue_read(CIRCLE_internal_queue_t* qp, int rank)
{
    if(!qp) {
        LOG(CIRCLE_LOG_ERR, "Libcircle queue not initialized.");
        return -1;
    }

    LOG(CIRCLE_LOG_DBG, "Reading from checkpoint file %d.", rank);

    if(qp->count != 0) {
        LOG(CIRCLE_LOG_WARN, \
            "Reading items from checkpoint file into non-empty work queue.");
    }

    char filename[256];
    sprintf(filename, "circle%d.txt", rank);

    LOG(CIRCLE_LOG_DBG, "Attempting to open %s.", filename);

    FILE* checkpoint_file = fopen(filename, "r");

    if(checkpoint_file == NULL) {
        LOG(CIRCLE_LOG_ERR, "Unable to open checkpoint file %s", filename);
        return -1;
    }

    LOG(CIRCLE_LOG_DBG, "Checkpoint file opened.");

    uint32_t len = 0;
    char str[CIRCLE_MAX_STRING_LEN];

    while(fgets(str, CIRCLE_MAX_STRING_LEN, checkpoint_file) != NULL) {
        len = strlen(str);

        if(len > 0) {
            str[len - 1] = '\0';
        }
        else {
            continue;
        }

        if(CIRCLE_internal_queue_push(qp, str) < 0) {
            LOG(CIRCLE_LOG_ERR, "Failed to push element on queue \"%s\"", str);
        }

        LOG(CIRCLE_LOG_DBG, "Pushed %s onto queue.", str);
    }

    return fclose(checkpoint_file);
}

/**
 * Write out the queue structure to a checkpoint file.
 *
 * @param qp the queue structure to be written to the checkpoint file.
 * @param rank the node which is writing out the checkpoint file.
 *
 * @return a positive value on success, negative otherwise.
 */
int8_t CIRCLE_internal_queue_write(CIRCLE_internal_queue_t* qp, int rank)
{
    LOG(CIRCLE_LOG_INFO, \
        "Writing checkpoint file with %d elements.", qp->count);

    if(qp->count == 0) {
        return 0;
    }

    char filename[256];
    sprintf(filename, "circle%d.txt", rank);
    FILE* checkpoint_file = fopen(filename, "w");

    if(checkpoint_file == NULL) {
        LOG(CIRCLE_LOG_ERR, "Unable to open checkpoint file %s", filename);
        return -1;
    }

    char str[CIRCLE_MAX_STRING_LEN];

    while(qp->count > 0) {
        if(CIRCLE_internal_queue_pop(qp, str) < 0) {
            LOG(CIRCLE_LOG_ERR, "Failed to pop item off queue.");
            return -1;
        }

        if(fprintf(checkpoint_file, "%s\n", str) < 0) {
            LOG(CIRCLE_LOG_ERR, "Failed to write \"%s\" to file.", str);
            return -1;
        }
    }

    return fclose(checkpoint_file);

}

/* EOF */
