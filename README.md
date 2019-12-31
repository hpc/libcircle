libcircle
=========
libcircle is an API for distributing embarrassingly parallel workloads using self-stabilization. Details on the algorithms used may be found at <http://dl.acm.org/citation.cfm?id=2389114>.

Dependencies
------------
* Open MPI  <http://www.open-mpi.org/>

Compile and install
-------------------
The current build status is: [![Build Status](https://travis-ci.org/hpc/libcircle.png?branch=master)](https://travis-ci.org/hpc/libcircle)

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

1. ```rpmbuild -ta libcircle-<version>.tar.gz```
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

To use the optional reduction:

1. Define and register three callback functions with libcircle:
 * CIRCLE_cb_reduce_init - this function is called once on each process for each reduction invocation to capture the initial contribution from that process to the reduction.
 * CIRCLE_cb_reduce_op - this function is called each time libcircle needs to combine two reduction values.
 * CIRCLE_cb_reduce_fini - this function is called once on the root process to output the final reduction result.
2. Update the value of reduction variable(s) within the CIRCLE_cb_process callback as work items are dequeued and processed by libcircle.

The libcircle library will periodically invoke a reduction if one has
registered the reduction callbacks.  The period between consecutive
reduction invocations is currently hard-coded to 10 seconds.  This
period will be configurable in a future release.

The example below shows how to count the number of items processed.
Each process counts the number of items it has processed locally.
The reducton computes the global sum across processes,
and it prints the sum along with the average rate.

```C
/*
 * a global variable to capture start time of reduction,
 * not necessary, but useful so we can compute an average rate
 */
static double reduce_start;

/*
 * a global variable to count number of items processed
 * on the local process
 */
static uint64_t reduce_count;

/*
 * The reduce_init callback provides the memory address and size of the variable(s)
 * to use as input on each process to the reduction operation.
 * One can specify an arbitrary block of data as input.  When a new
 * reduction is started, libcircle invokes this callback on each
 * process to snapshot the memory block specified in the call to CIRCLE_reduce.
 * The library makes a memcpy of this memory block given to CIRCLE_reduce.
 */
static void reduce_init(void)
{
    /*
     * We give the starting memory address and size of a memory
     * block we want libcircle to capture on this process when
     * it starts a new reduction operation.
     *
     * In this example, we capture a single uint64_t value.
     */
    CIRCLE_reduce(&reduce_count, sizeof(uint64_t));
}

/*
 * On intermediate nodes of the reduction tree, libcircle invokes the
 * reduce_exec callback to reduce two data buffers.  The starting
 * address and size of each data buffer are provided as input parameters
 * to the callback function.  An arbitrary reduction operation can
 * be executed.  Then libcircle snapshots the memory block specified
 * in the call to CIRCLE_reduce to capture the partial result.
 * The library makes a memcpy of this memory block.
 *
 * Note that the sizes of the input buffers do not have to be the same.
 * The output buffer does not need to be the same size as either input
 * buffer.  For example, one could contentate buffers so that the
 * reduction operates more like a gather.
 */
static void reduce_exec(const void* buf1, size_t size1, const void* buf2, size_t size2)
{
    /*
     * Here we are given the starting address and size of two input buffers.
     * These could be the intial memory blocks captured in reduce_init,
     * or they could be intermediate results captured from a reduce_exec call.
     *
     * In this example, we sum two input uint64_t values and
     * save a copy the result by calling CIRCLE_reduce.
     */
    const uint64_t* a = (const uint64_t*) buf1;
    const uint64_t* b = (const uint64_t*) buf2;
    uint64_t val = a[0] + b[0];
    CIRCLE_reduce(&val, sizeof(uint64_t));
}

/*
 * The reduce_fini callback is only invoked on the root process.  It
 * provides a buffer holding the final reduction result as in input
 * parameter. Typically, one might print the result in this callback.
 *
 * In this example, we compute the average processing rate,
 * and we print the rate and global count of items processed.
 */
static void reduce_fini(const void* buf, size_t size)
{
    // get result of reduction
    uint64_t count = *(const uint64_t*) buf;

    // get current time
    double now = MPI_Wtime();

    // compute average processing rate
    double rate = 0.0;
    double secs = now - reduce_start;
    if (secs > 0.0) {
        rate = (double)count / secs;
    }

    // print status to stdout
    MFU_LOG(MFU_LOG_INFO, "Processed %llu items in %f secs (%f items/sec) ...",
        (unsigned long long)count, secs, rate);
}

/*
 * Modify our CIRCLE_cb_process callback to increment our reduction
 * counter, e.g., to count number of items processed.
 */
void my_process_some_work(CIRCLE_handle *handle)
{
    handle->dequeue(&my_data);

    // Do stuff with my_data ...

    /*
     * Update the variable we use to count the number of
     * items we have processed.  libcircle will periodically
     * snapshot this variable as input to a reduction operation
     * due to our reduce_init callback that provides the memory
     * address and size of this variable.
     */
    reduce_count++;
}

/*
 * Initialize variables for reductions
 */
reduce_start = MPI_Wtime(); // capture the start time
reduce_count = 0; // set our count to 0

/*
 * Register our 3 reduction callback functions.
 * This enables the reduction operations.
 */
CIRCLE_cb_reduce_init(&reduce_init);
CIRCLE_cb_reduce_op(&reduce_exec);
CIRCLE_cb_reduce_fini(&reduce_fini);

/*
 * Then do all of the regular libcircle stuff
 */
CIRCLE_cb_create(&my_create_some_work);
CIRCLE_cb_process(&my_process_some_work);
CIRCLE_begin();
CIRCLE_finalize();

```
