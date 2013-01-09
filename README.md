libcircle
=========
libcircle is an API for distributing embarrassingly parallel workloads using self-stabilization. Details on the algorithms used may be found at <http://dl.acm.org/citation.cfm?id=2389114>.

Dependencies
------------
* Open MPI  <http://www.open-mpi.org/>

Compile and install
-------------------
```
./configure
make all check
sudo make install
```

To enable output from libcircle (including fatal errors), run configure with
"--enable-loglevel=number" where "number" is one of the following options:

* "1" fatal errors only.
* "2" errors and lower log levels.
* "3" warnings and lower log levels.
* "4" info messages on internal operations and lower log levels.
* "5" fine grained debug messages and lower log levels.

RPM Build and Install
---------------------
To build an RPM, use the following instructions:

1. ```rpmbuild -ta libcircle-<version>-<release>.tar.gz```
2. ```rpm --install <the appropriate RPM files>```

Developer API Information
-------------------------
The basic program flow when using libcircle is the following:

1. Define callbacks which enqueue or dequeue strings from the queue.
2. Execute the program.

```C
#include <libcircle.h>

/* An example of a create callback defined by your program */
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

/* An example of a process callback defined by your program. */
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
 * Initialize state required by libcircle. Arguments should be those passed in
 * by the launching process. argc is a pointer to the number of arguments,
 * argv is the argument vector. The return value is the MPI rank of the current
 * process.
 */
int rank = CIRCLE_init(&argc, argv);

/*
 * Processing and creating work is done through callbacks. Here's how we tell
 * libcircle about our function which creates the initial work. For MPI nerds,
 * this is your rank 0 process.
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
