#ifndef LIBCIRCLE_H
#define LIBCIRCLE_H

/* FIXME: need to calculate this better at some point. */
#define MAX_STRING_LEN 2048*sizeof(char)

/*
 * The interface to the work queue. This can be accessed from within the
 * process and create work callbacks.
 */
typedef struct {
    void (*enqueue)(char *element);
    void (*dequeue)(char *element);
} CIRCLE_handle;

/*
 * The type for defining callbacks for create and process.
 */
typedef void (*CIRCLE_cb)(CIRCLE_handle *handle);

/*
 * Initialize internal state needed by libcircle. This should be called before
 * any other libcircle API call.
 */
void CIRCLE_init(void);

/*
 * Processing and creating work is done through callbacks. Here's how we tell
 * libcircle about our function which creates work.
 */
void CIRCLE_cb_create(CIRCLE_cb func);

/*
 * After you give libcircle a way to create work, you need to tell it how that
 * work should be processed.
 */
void CIRCLE_cb_process(CIRCLE_cb func);

/*
 * Once you've defined and told libcircle about your callbacks, use this to
 * execute your program.
 */
void CIRCLE_begin(void);

/*
 * After your program has executed, give libcircle a chance to clean up after
 * itself by calling this. This should be called after all libcircle API calls.
 */
void CIRCLE_finalize(void);

#endif /* LIBCIRCLE_H */
