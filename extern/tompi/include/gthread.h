/* Thread include file to support various threads systems.  This file defines
 * the same set of macros for various underlying thread systems, allowing one
 * to write more portable thread programs, without any loss of efficiency.  It
 * is intended for general use but was written in particular for TOMPI.
 *
 * The following thread systems are supported.  To choose one, simply
 * change the definition of GTHREAD_SYSTEM to one of the defined constants.
 *
 *   - Pthreads (POSIX threads)
 *       This refers to the final draft, that is, IEEE Std. 1003.1c-1995,
 *       or _POSIX_VERSION == 199506L.  See
 *           http://www.sun.com/workshop/threads/posix.html
 *       Examples: AIX 4.1.x, AIX 4.2.x, SunOS 5.5.
 *
 *   - Old pthreads (create/delete instead of init/destroy)
 *       Unfortunately, this won't work right now; some &'s must be removed.
 *       Examples: AIX pthreads 1.13 (AIX 3.2.4)
 *
 *   - Solaris threads
 *       Examples: SunOS 5.4.
 *
 *   - Cthreads (AKA ithreads)
 *       User-level threads package.  Unfortunately, it has some problems on
 *       AIX (at least): each context switch adds to the stack, eventually
 *       causing stack overflow.  This means you have to allocate a reasonably
 *       big stack.  It also has trouble running more than ten threads.
 *       Available from http://www.cc.gatech.edu/systems/projects/Cthreads/
 *
 *   - RT threads (real-time threads)
 *       User-level threads package supporting AIX, SunOS 4/5, FreeBSD, and
 *       WindowsNT.
 *       Unfortunately, it seems to have some (rather strange) problems with
 *       threads' stacks on AIX.
 *       Available from http://www.cs.ubc.ca/nest/dsg/cmfs/rtt.html
 */

#ifndef __GTHREAD_H__
#define __GTHREAD_H__

#define PTHREADS 1
#define OLD_PTHREADS 2
#define SOLARIS_THREADS 3
#define CTHREADS 4
#define RT_THREADS 5
#define GTHREAD_SYSTEM PTHREADS

/************ No need to look further for configuration purposes ************/

/* For using these macros outside of TOMPI:
 *
 * Exactly one of the modules including it must define DEFINE_GTHREAD_GLOBALS
 * before including it.  This defines some global variables.  Also, you may
 * want to change MPII_ to a more reasonable prefix for your purposes.  If you
 * define MPII_STACK_SIZE and declare MPII_Stack_size, it may use this to
 * specify the stack size in bytes.  Similarly, the can define MPII_NUM_PROC
 * and declare MPII_Num_proc for the number of processors to use.  The
 * underlying thread system has to support these, of course (otherwise they
 * are ignored).  Note that they are explored either when init_threads() is
 * called or when spawn_thread() is called.  Values of zero cause the defaults.
 *
 * Provided macros:
 * 
 * void init_threads ()
 *   - Must be called before any other macros
 * void thread_error (char *prefix, int last_return)
 *   - Prints the error of the last operation, prefixed with string and ": "
 *   - On some systems, requires you to pass the last (non-zero) return value
 *     of the error.
 *
 * Thread gives the type of a thread id.  Macros for manipulation:
 *
 * int spawn_thread (void *(*fun) (void *), void *arg, Thread thread)
 *   - Creates a new thread (storing the id in the variable thread) which
 *     calls function fun with argument arg.
 * void exit_thread (void *rval)
 *   - Exits the calling thread with return value rval.
 * Thread thread_id ()
 *   - Returns the id of the calling thread.
 * int join_thread (Thread thread, void *rval)
 *   - Waits for thread to exit
 *   - Sets the variable rval to the return value of that thread (on success)
 *   - OPTIONAL: only used by waiter thread (see below)
 *
 * Key gives the type of a thread-specific datum (TSD).  Macros for
 * manipulation:
 *
 * int new_tsd (Key key)
 *   - Sets the variable key to a new TSD key.
 * void *get_tsd (Key key)
 *   - Returns the TSD associated with the given key.  (Initially NULL.)
 * int set_tsd (Key key, void *val)
 *   - Sets the TSD associate with key key to val.
 *
 * Mutex gives the type of a mutex (binary semaphore).  Macros for
 * manipulation:
 *
 * int new_mutex (Mutex mutex)
 *   - Sets the variable mutex to a new mutex.
 * int lock (Mutex mutex)
 *   - Locks the given mutex, blocking until it is available.
 * int unlock (Mutex mutex)
 *   - Unlocks the given mutex.
 * int delete_mutex (Mutex mutex)
 *   - Deletes the given mutex (opposite of new_mutex).
 *
 * Cond gives the type for condition variables.  Macros for manipulation:
 *
 * int new_cond (Cond cond)
 *   - Sets the variable cond to a new condition variable.
 * int notify (Cond cond)
 *   - Sends a single signal on the condition variable, notifying one waiting
 *     thread.
 * int notify_all (Cond cond)
 *   - Sends a broadcast signal on the condition variable, notifying all
 *     waiting threads.
 * int wait (Cond cond, Mutex mutex)
 *   - Atomically unlocks mutex, waits for a signal on cond, and relocks mutex.
 * int delete_cond (Cond cond)
 *   - Deletes the given cond (opposite of new_cond).
 *
 * Any macros that have an undocumented int return value return 0 iff the
 * call succeeded.
 *
 * The following informational macros are also defined.
 *   - GTHREAD_SYSTEM_NAME: a string giving a simple name of the thread system
 *
 * Some thread systems (most of those supported by the operating system) allow
 * the initial call to main() to be a thread.  Others (most user-level thread
 * systems) have the problem that when main() returns, the whole program exits.
 * Some systems solve this by forcing the user to redefine main to something
 * else.  In this case, we #define main to the appropriate function name (this
 * is caught by mkthreadspec, which puts the definition in threadspec.h).  In
 * other systems, we have to write a special main() (called the waiter thread)
 * that waits for the other threads to finish, and then exits.  In this case,
 * MPII_WAITER_THREAD is defined, and the join_thread() macro must be provided.
 * This also has the effect of renaming main; it is defined to MPII_Main.
 */

/* Custom thread-specific data, defined in src/misc/tsd.c.  MPII_TSD may be
 * defined to one of these values for custom thread-specific data.  This
 * feature is used when the thread system doesn't support several
 * thread-specific data or the support is broken.
 */
#define MPII_TSD_ALMOST_INTERVAL 1    /* Excluding first thread id, thread ids
                                         form interal of integers. */
#define MPII_TSD_SINGLE 2             /* Single thread-specific data is defined
                                         by get_tsd1 and set_tsd1. */

#if GTHREAD_SYSTEM == OLD_PTHREADS
#   define pthread_attr_init pthread_attr_create
#   define pthread_attr_destroy pthread_attr_delete
#   define pthread_mutexattr_init pthread_mutexattr_create
#   define pthread_mutexattr_destroy pthread_mutexattr_delete
#   define pthread_condattr_init pthread_condattr_create
#   define pthread_condattr_destroy pthread_condattr_delete
#   define pthread_key_create pthread_keycreate
#   define GTHREAD_SYSTEM PTHREADS      /*NOTREALTHREADSYSTEM*/
#endif

#ifdef DEFINE_GTHREAD_GLOBALS
    void *MPII_temp_ptr;
    int MPII_temp_int;
#else
    extern void *MPII_temp_ptr;
    extern int *MPII_temp_int;
#endif

#ifndef MPII_STACK_SIZE
#   define MPII_Stack_size 0
#endif
#ifndef MPII_NUM_PROC
#   define MPII_Num_proc 0
#endif

/* If the system does not provide a default, we use this default (in bytes). */
#define MPII_DEFAULT_STACK_SIZE 1000000

/****************************************************************************/

#if GTHREAD_SYSTEM == PTHREADS
#   include <pthread.h>

#   define GTHREAD_SYSTEM_NAME "POSIX threads"

#   ifdef DEFINE_GTHREAD_GLOBALS
        pthread_attr_t MPII_temp_attrib;
#   else
        extern pthread_attr_t MPII_temp_attrib;
#   endif

#   ifdef _POSIX_THREAD_ATTR_STACKSIZE
#     define init_threads() \
        do { \
          pthread_attr_init (&MPII_temp_attrib); \
          if (MPII_Stack_size > 0) \
            pthread_attr_setstacksize (&MPII_temp_attrib, MPII_Stack_size); \
        } while (0)
#   else
#     define init_threads() pthread_attr_init (&MPII_temp_attrib)
#   endif
#   define thread_error(prefix,error) perror (prefix)

#   define Thread pthread_t
#   define spawn_thread(fun,arg,thread) \
          pthread_create (&(thread), &MPII_temp_attrib, fun, arg)
#   define exit_thread(i) pthread_exit ((void *) (i))
#   define thread_id() pthread_self ()
#   define join_thread(thread,rval) pthread_join (thread, &(rval))

#   define Key pthread_key_t
#   define new_tsd(key) pthread_key_create (&(key), NULL)
#   define get_tsd(key) pthread_getspecific (key)
#   define set_tsd(key,val) pthread_setspecific (key, val)

#   define Mutex pthread_mutex_t
#   define new_mutex(mutex) pthread_mutex_init (&(mutex), NULL)
#   define lock(mutex) pthread_mutex_lock (&(mutex))
#   define unlock(mutex) pthread_mutex_unlock (&(mutex))
#   define delete_mutex(mutex) pthread_mutex_destroy (&(mutex))

#   define Cond pthread_cond_t
#   define new_cond(cond) pthread_cond_init (&(cond), NULL)
#   define notify(cond) pthread_cond_signal (&(cond))
#   define notify_all(cond) pthread_cond_broadcast (&(cond))
#   define wait(cond,mutex) pthread_cond_wait (&(cond), &(mutex))
#   define delete_cond(cond) pthread_cond_destroy (&(cond))

#   define MPII_WAITER_THREAD

/****************************************************************************/

#elif GTHREAD_SYSTEM == SOLARIS_THREADS
#   include <thread.h>
#   include <synch.h>

#   define GTHREAD_SYSTEM_NAME "Solaris threads"

#   define init_threads() \
        do { \
            if (MPII_Num_proc > 0) \
                thr_setconcurrency (MPII_Num_proc); \
        } while (0)
#   define thread_error(prefix,error) perror (prefix)

#   define Thread thread_t
#   define spawn_thread(fun,arg,thread) \
          thr_create (NULL, MPII_Stack_size, fun, arg, 0, &(thread))
#   define exit_thread(i) thr_exit ((void *) (i))
#   define thread_id() thr_self ()
#   define join_thread(thread,rval) thr_join (thread, NULL, &(rval))

#   define MPII_TSD MPII_TSD_ALMOST_INTERVAL       /* Solaris TSD is broken. */
#   define Key thread_key_t
#   define new_tsd(key) thr_keycreate (&(key), NULL)
#   define get_tsd(key) (thr_getspecific (key, &MPII_temp_ptr), MPII_temp_ptr)
#   define set_tsd(key,val) thr_setspecific (key, val)

#   define Mutex mutex_t
#   define new_mutex(mutex) mutex_init (&(mutex), USYNC_THREAD, 0)
#   define lock(mutex) mutex_lock (&(mutex))
#   define unlock(mutex) mutex_unlock (&(mutex))
#   define delete_mutex(mutex) mutex_destroy (&(mutex))

#   define Cond cond_t
#   define new_cond(cond) cond_init (&(cond), USYNC_THREAD, 0)
#   define notify(cond) cond_signal (&(cond))
#   define notify_all(cond) cond_broadcast (&(cond))
#   define wait(cond,mutex) cond_wait (&(cond), &(mutex))
#   define delete_cond(cond) cond_destroy (&(cond))

#   define MPII_WAITER_THREAD

/****************************************************************************/

#elif GTHREAD_SYSTEM == CTHREADS
#   include <cthread.h>

#   define GTHREAD_SYSTEM_NAME "Cthreads"

    /* DEFAULT_MEM is typically 10 (implying 2^10 = 1 meg) of "memory" for
     * mutices and such.  This isn't enough most of the time.  I've raised it
     * to 12 here, which seems to work up to 10 threads (on an AIX box).
     * 14 doesn't help at all :/.
     */
#   define init_threads() \
        do { \
            struct configuration config; \
            config.stack_size = (MPII_Stack_size > 0 ? MPII_Stack_size \
                                                     : DEFAULT_STACK); \
            config.memory_exponent = 12/*DEFAULT_MEM*/; \
            config.threads_per_proc = DEFAULT_THREADS; \
            cthread_configure (&config, PUT_CONFIG); \
            cthread_start ((MPII_Num_proc > 0 ? MPII_Num_proc : 1)); \
        } while (0)
#   define thread_error(prefix,error) cthread_perror (prefix, error)

#   define Thread cthread_t
#   define spawn_thread(fun,arg,thread) \
          (((thread) = cthread_fork (fun, arg, N_ANYWHERE)) == 0)
#   define exit_thread(i) cthread_exit ((void *) (i))
#   define thread_id() cthread_self ()
#   define join_thread(thread,var) cthread_join (thread, &(var))

    /* Cthreads doesn't support more than one thread-specific data. */
#   define MPII_TSD MPII_TSD_SINGLE
#   define get_tsd1() cthread_data (cthread_self ())
#   define set_tsd1(val) (cthread_set_data (cthread_self (), val), 0)

#   define Mutex mutex_t
#   define new_mutex(mutex) mutex_alloc (&(mutex), N_CURRENT)
#   define lock(mutex) (mutex_lock (mutex), 0)
#   define unlock(mutex) (mutex_unlock (mutex), 0)
#   define delete_mutex(mutex) (mutex_free (mutex), 0)

#   define Cond condition_t
#   define new_cond(cond) condition_alloc (&(cond), N_CURRENT)
    /* condition_signal only returns T_SUCCEED (0) or T_NO_THREAD_QUEUED when
     * it didn't do anything -- we don't want such a distinction.
     */
#   define notify(cond) (condition_signal (cond), 0)
    /* The following just return void. */
#   define notify_all(cond) (condition_broadcast (cond), 0)
#   define wait(cond,mutex) (condition_wait (cond, mutex), 0)
#   define delete_cond(cond) (condition_free (cond), 0)

    /* I think this can be avoided using cthread_init instead of cthread_start
     * (which may also be more inter-Cthread-version friendly).
     */
#   define MPII_WAITER_THREAD

/****************************************************************************/

#elif GTHREAD_SYSTEM == RT_THREADS
    /* rtthreads.h defines exit without checking for its existance. */
#   ifdef exit
#     undef exit
#   endif
#   include <rtthreads.h>

#   define GTHREAD_SYSTEM_NAME "Real-time (RT) threads"

#   ifdef DEFINE_GTHREAD_GLOBALS
      RttThreadId MPII_Rtt_id;
      RttSchAttr MPII_Rtt_attr;
#   else
      extern RttThreadId MPII_Rtt_id;
      extern RttSchAttr MPII_Rtt_attr;
#   endif

#   define init_threads() \
      do { \
        MPII_Rtt_attr.startingtime = RTTZEROTIME;  /* start now */ \
        MPII_Rtt_attr.priority = RTTNORM;          /* normal priority */ \
        MPII_Rtt_attr.deadline = RTTNODEADLINE;    /* no rush */ \
        /* Make the main thread (from mainp()) the same priority as others */ \
        RttSetThreadSchedAttr (RttMyThreadId (), MPII_Rtt_attr); \
      } while (0)
#   define thread_error(prefix,error) fprintf (stderr, "%s: RT threads error\n", prefix);

#   define Thread RttThreadId
#   define spawn_thread(fun,arg,thread) \
      (RttCreate (&MPII_Rtt_id, fun, (MPII_Stack_size > 0 ? MPII_Stack_size : \
         MPII_DEFAULT_STACK_SIZE), "TOMPI", arg, MPII_Rtt_attr, RTTUSR) \
       != RTTOK)
#   define exit_thread(i) RttExit ()
#   define thread_id() RttMyThreadId ()

    /* RT threads doesn't support more than one thread-specific data. */
#   define MPII_TSD MPII_TSD_SINGLE
#   define get_tsd1() (RttGetData (RttMyThreadId (), &MPII_temp_ptr), MPII_temp_ptr)
#   define set_tsd1(val) RttSetData (RttMyThreadId (), val)

    /* Semaphores are the only synchronization primitives in RT threads. */
#   define Sem RttSem
#   define new_sem(sem,initial) RttAllocSem (&(sem), initial, RTTFCFS)
#   define sem_down(sem) RttP (sem)
#   define sem_up(sem) RttV (sem)
#   define delete_sem(sem) RttFreeSem (sem)
#   define MPII_MUTEX_WITH_SEM
#   define MPII_COND_WITH_SEM

    /* RT threads requires the main program to be in mainp() instead of
     * main().
     */
#   define main mainp

#endif

/****************************************************************************/

/* Custom thread-specific data */

#ifdef MPII_TSD
#   ifdef Key
#       undef Key
#   endif
#   ifdef new_tsd
#       undef new_tsd
#   endif
#   ifdef get_tsd
#       undef get_tsd
#   endif
#   ifdef set_tsd
#       undef set_tsd
#   endif

#   if MPII_TSD == MPII_TSD_ALMOST_INTERVAL
      /* A Key is an array of void *'s */
      typedef void **Key;
#   elif MPII_TSD == MPII_TSD_SINGLE
      typedef int Key;
#   endif

#   define new_tsd(key) MPII_New_tsd (&(key))
#   define get_tsd(key) MPII_Get_tsd (&(key))
#   define set_tsd(key,val) MPII_Set_tsd (&(key),val)
#endif

/****************************************************************************/

/* Support for when only semaphores are provided */

/* Implementation of mutices with semaphores */
#ifdef MPII_MUTEX_WITH_SEM
#   define Mutex Sem
#   define new_mutex(sem) new_sem (sem, 1)
#   define lock(sem) sem_down (sem)
#   define unlock(sem) sem_up (sem)
#   define delete_mutex(sem) delete_sem (sem)
#endif

/* Implementation of condition variables with mutices */
#ifdef MPII_COND_WITH_SEM
    typedef struct {
      enum {COND_WAIT, COND_NOTIFY, COND_NOTIFY_ALL} signal;
      int waiting;      /* number of processes waiting on condition */
      Sem sem;
    } Cond;
#   define new_cond(cond) ((cond).waiting = 0, new_sem ((cond).sem, 0))
#   define notify(cond) ((cond).signal = ((cond).signal == COND_NOTIFY_ALL ? \
                                          COND_NOTIFY_ALL : COND_NOTIFY), \
                         sem_up ((cond).sem))
#   define notify_all(cond) ((cond).signal = COND_NOTIFY_ALL, \
                             sem_up ((cond).sem))
#   define wait(cond,mutex) MPII_Wait_with_sem (&(cond), &(mutex))
#   define delete_cond(cond) delete_sem ((cond).sem)

#   ifdef DEFINE_GTHREAD_GLOBALS
      int MPII_Wait_with_sem (Cond *c, Mutex *m)
      {
        /* This ignores notify()s that happened when no one was waiting, and
         * terminates notify_all()s when we get down to the end (i.e., everyone
         * has been notified).
         */
        if (c->waiting <= 0)
          c->signal = COND_WAIT;

        c->waiting++;             /* Start getting notify messages */
        while (c->signal == COND_WAIT)  /* In case the semaphore already has a
                                         positive value and need to clear it */
        {
          unlock (*m);            /* Have to do this before we block */
          sem_down (c->sem);      /* Block until we get a notify message */
          lock (*m);              /* Re-lock c (as required) */
        }
        if (c->signal == COND_NOTIFY)
          c->signal = COND_WAIT;  /* Clear the notify() message */
        else if (c->signal == COND_NOTIFY_ALL)
          sem_up (c->sem);        /* Pass notify_all() message down */
        c->waiting--;             /* Preserve number of waiters for above */

        return 0;
      }
#   else
      extern int MPII_Wait_with_sem (Cond *c, Mutex *m);
#   endif
#endif

/****************************************************************************/

/* If requested, define a thread-local version of exit.  This has to go down
 * here because some thread packages define exit themselves.
 */
#ifdef MPII_THREAD_LOCAL_EXIT
#  ifdef exit
#    undef exit
#  endif
#  define exit(val) exit_thread (val)
#endif

/****************************************************************************/

/* Having a waiter thread causes renaming of main to MPII_Main.  The waiter
 * thread takes place of main().
 */
#ifdef MPII_WAITER_THREAD
#  define main MPII_Main
#endif

/****************************************************************************/

#endif
