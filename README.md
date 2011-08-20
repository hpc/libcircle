libcircle 0.0.1
===============

__NOTE: This is not complete. Please don't develop any software against it yet. Thanks!__

libcircle is an API for distributing embarrassingly parallel workloads using self-stabilization. Local actions work towards a global objective in a finite number of operations. It is not meant for applications where cross-process communication is necessary. The core algorithm used is based on Dijkstra's 1974 token ring proposal and heavily uses MPI under the hood.

How does this thing work?
-------------------------
```C
#include <libcircle.h>

void create_some_work(CIRCLE_handle *handle)
{
    /*
     * This is where you should generate work that needs to be processed.
     * For example, if your goal is to lstat files on a large filesystem,
     * this is where you would readdir() and and enqueue directory names.
     *
     * This should be a small amount of work. For example, only enqueue the
     * filenames from a single directory.
     */
    while((data_to_process = readdir(...)) != NULL)
    {
        handle->enqueue((void *)data_to_process);
    }
}

void process_some_work(CIRCLE_handle *handle)
{
    /*
     * This is where work should be processed. For example, this is where you
     * should lstat one of the files which was placed on the queue by your
     * create_some_work callback. Again, you should try to keep this short and
     * block as little as possible.
     */
    (char *)my_data = handle->dequeue();
    ...
    finished_work = lstat(my_data, ...);
    ...
    store_in_database(finished_work);
}

/*
 * This handle holds internal state needed by libcircle, such as a reference
 * to the queue data structures.
 */
CIRCLE_handle *handle;
handle = CIRCLE_create();

/*
 * Processing and creating work is done through callbacks. Here's how we tell
 * libcircle about our function which creates work.
 */
void (*create_some_work)(CIRCLE_handle *handle);
handle->create_work(&create_some_work);

/*
 * After you give libcircle a way to create work, you need to tell it how that
 * work should be processed.
 */
void (*process_some_work)(CIRCLE_handle *handle);
handle->define_process_work(&process_some_work);

/*
 * Now that everything is setup, lets fire it up.
 */
status = CIRCLE_begin(handle);

/*
 * Finally, free the libcircle context with the function provided by the API.
 */
CIRCLE_free(handle);
```
