libcircle 0.0.1
===============

__NOTE: This is not complete. Please don't develop any software against it yet. Thanks!__

libcircle is an API for distributing embarrassingly parallel workloads using self-stabilization. Local actions work towards a global objective in a finite number of operations. It is not meant for applications where cross-process communication is necessary. The core algorithm used is based on Dijkstra's 1974 token ring proposal and heavily uses MPI under the hood libcircle should be easy to use for anyone with a basic grasp of map-reduce (assuming a predefined assumed mapping process).

How does this thing work?
-------------------------
```C
#include <libcircle.h>

void my_create_some_work(CIRCLE_handle *handle)
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
        handle->enqueue(data_to_process);
    }
}

void my_process_some_work(CIRCLE_handle *handle)
{
    /*
     * This is where work should be processed. For example, this is where you
     * should lstat one of the files which was placed on the queue by your
     * create_some_work callback. Again, you should try to keep this short and
     * block as little as possible.
     */
    handle->dequeue(&my_data);
    ...
    finished_work = lstat(my_data, ...);
    ...
    store_in_database(finished_work);
}

/*
 * Initialize state required by libcircle.
 */
CIRCLE_init();

/*
 * Processing and creating work is done through callbacks. Here's how we tell
 * libcircle about our function which creates work.
 */
CIRCLE_cb_create(&my_create_some_work);

/*
 * After you give libcircle a way to create work, you need to tell it how that
 * work should be processed.
 */
CIRCLE_cb_process(&my_process_some_work);

/*
 * Now that everything is setup, lets execute everything.
 */
CIRCLE_begin();

/*
 * Finally, give libcircle a chance to clean up after itself.
 */
CIRCLE_finalize();
```
