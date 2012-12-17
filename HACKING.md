Things that would be nice to have in libcircle
==============================================

## Variable-Size Queue Items

Right now, the queue item size is fixed by the CIRCLE_MAX_STRING_LEN constant.
It would be very nice if this size would be variable so the API could allow
for passing in a length and a buffer for the queue item rather than the
current fixed-size design. Things like overhead to the MPI message size need
to be considered so performance impact is minimized. Also, the internal queue
structures would need to be designed differently. Performance should be
considered when attempting to redesign the internal queue structure.

## Multi-Queue Support

It would be useful to implement queue item tagging as part of the API. This
would allow for providing a developer with the illusion of multiple queues
without the overhead of actually running multiple instances of the libcircle
algorithm. This would be trivial to implement once variable-size queue items
is implemented.

## Fault Tolerance

There are several ways that fault tolerance may be implemented. One idea is to
create a supervisor-worker scheme where each node is always a worker and
supervisor. In the case where an odd number of nodes is used, a single node
would act as two supervisors to other nodes. The worker part of each node
performs the same tasks as required by the existing algorithm. The supervisor
part of each node holds a copy of the queue items for the node it is
supervising (without performing any work on them). If the node it is
supervising dies, the supervisor atomically broadcasts to all other nodes what
the current state of things is (using some atomic broadcast algorithm, perhaps
using the termination ring?) and adds the copy of the work that the node held
to the queue in the worker part of the supervisor node. Things then continue
as usual. If the node being supervised finishes all work, the supervisor node
discards the copy of the queue items. The entire supervisor-worker scheme
should be designed so configurable k-failures can be tolerated. This requires
a node to support (n - 1) supervisors where n is the number of nodes in the
libcircle communicator. Supervisors should have the ability to serialize the
backup copy of the worker queue to disk, especially in the case where (k > 1)
since memory will be scarce.
