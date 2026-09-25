/**
 * @file
 *
 * Regression test for the blocking sends that used to live on the work
 * transfer path.
 *
 * A process that blocks in an MPI send stops servicing incoming
 * messages, which stalls every process waiting on it.  Two ways that
 * used to hang a job:
 *
 *   1. The work reply is large enough to go rendezvous, so the send
 *      does not complete until the peer posts a matching receive.  Any
 *      cycle in the "who asked whom for work" graph whose members all
 *      hold work at the time they service the request deadlocks.  The
 *      graph is built from random choices, so cycles are the norm.
 *
 *   2. Even a tiny send blocks when the transport is backed up.  The
 *      process then stops receiving, which backs up its own peers, and
 *      the stall spreads until the termination allreduce can no longer
 *      complete.
 *
 * This test drives the pattern that produces case 1: each process
 * repeatedly empties its queue, asks for work, refills, and then
 * services the request that arrived while it was busy.  Run it with a
 * tiny eager limit (see deadlock_test.sh) so that essentially every
 * work reply goes rendezvous, and the hang shows up in seconds.  The
 * driver script imposes the timeout; this program only checks that
 * every item that was created also got processed.
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcircle.h"

/* Length of each work item.  Long items make the data portion of a
 * work reply large, which is what pushes it past the eager limit. */
#define ITEM_LEN 900

/* Number of items rank 0 seeds the queue with. */
#define SEED_ITEMS 2000

/* Stop creating new work once this many items have been processed
 * locally.  Every process applies this independently, so the job ends
 * after roughly ranks * ITEM_BUDGET items. */
#define ITEM_BUDGET 4000

/* Number of items to enqueue for each one we process, while we are
 * still under budget.  Greater than one so queues keep refilling and
 * work keeps being redistributed. */
#define FANOUT 2

static long items_processed;
static long items_created;

static void make_item(char* buf, long id)
{
    int n = snprintf(buf, ITEM_LEN, "/deadlock/test/item/%ld/", id);

    /* snprintf returns what it would have written, so clamp it */
    if(n < 0) {
        n = 0;
    }
    else if(n > ITEM_LEN - 1) {
        n = ITEM_LEN - 1;
    }

    /* pad out to ITEM_LEN so the transfers are big */
    memset(buf + n, 'x', (size_t)(ITEM_LEN - 1 - n));
    buf[ITEM_LEN - 1] = '\0';
}

static void create_work(CIRCLE_handle* handle)
{
    char item[ITEM_LEN];
    long i;

    for(i = 0; i < SEED_ITEMS; i++) {
        make_item(item, i);
        handle->enqueue(item);
        items_created++;
    }
}

static void process_work(CIRCLE_handle* handle)
{
    char item[CIRCLE_MAX_STRING_LEN];

    if(handle->dequeue(item) < 0) {
        return;
    }

    items_processed++;

    if(items_processed >= ITEM_BUDGET) {
        /* we've done our share, stop making more work so the job can
         * drain and terminate */
        return;
    }

    int i;

    for(i = 0; i < FANOUT; i++) {
        char next[ITEM_LEN];
        make_item(next, items_created);
        handle->enqueue(next);
        items_created++;
    }
}

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int rank, ranks;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);

    /* One rank shares no work, so the test would pass without exercising
     * anything it is meant to cover.  Launchers do land us here: Ubuntu's
     * MPICH links against PMIx while its hydra hands out PMI-1, so every
     * rank falls back to singleton init and "mpirun -n 16" quietly becomes
     * 16 separate one-rank jobs. */
    if(ranks < 2) {
        if(rank == 0) {
            fprintf(stderr, "SKIP: need at least 2 ranks to share work, got %d\n",
                    ranks);
        }

        MPI_Finalize();
        return 77;
    }

    /* the flags mpifileutils uses */
    CIRCLE_init(argc, argv, CIRCLE_SPLIT_EQUAL | CIRCLE_TERM_TREE);
    CIRCLE_enable_logging(CIRCLE_LOG_ERR);

    CIRCLE_cb_create(&create_work);
    CIRCLE_cb_process(&process_work);

    /* If the library still blocks in a send, this call never returns
     * on at least one process and the driver script kills the job on
     * its timeout. */
    CIRCLE_begin();

    CIRCLE_finalize();

    /* every item that was created must have been processed exactly
     * once, no matter which process ended up doing it */
    long total_created = 0;
    long total_processed = 0;
    MPI_Reduce(&items_created, &total_created, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&items_processed, &total_processed, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    int rc = 0;

    if(rank == 0) {
        printf("ranks %d created %ld processed %ld\n",
               ranks, total_created, total_processed);

        if(total_created != total_processed) {
            fprintf(stderr, "FAIL: created %ld items but processed %ld\n",
                    total_created, total_processed);
            rc = 1;
        }
    }

    MPI_Bcast(&rc, 1, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Finalize();

    return rc;
}
