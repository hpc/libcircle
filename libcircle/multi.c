#include <libcircle.h>

CIRCLE_cb CIRCLE_multi_jump_start[CIRCLE_MAX_QUEUE_COUNT];
CIRCLE_cb CIRCLE_multi_jump_process[CIRCLE_MAX_QUEUE_COUNT];

__inline__ int CIRCLE_multi_add_start(unsigned int queue_id, CIRCLE_cb func)
{
    return _CIRCLE_multi_add(queue_id, func, &CIRCLE_multi_jump_start);
}

__inline__ int CIRCLE_multi_add_process(unsigned int queue_id, CIRCLE_cb func)
{
    return _CIRCLE_multi_add(queue_id, func, &CIRCLE_multi_jump_process);
}

__inline__ int CIRCLE_multi_exec_start(\
                                       unsigned int queue_id, CIRCLE_handle* handle)
{
    return _CIRCLE_multi_exec(queue_id, handle, &CIRCLE_multi_jump_start);
}

__inline__ int CIRCLE_multi_exec_process(\
        unsigned int queue_id, CIRCLE_handle* handle)
{
    return _CIRCLE_multi_exec(queue_id, handle, &CIRCLE_multi_jump_process);
}

int _CIRCLE_multi_add(unsigned int queue_id, CIRCLE_cb func, CIRCLE_cb* jump)
{
    if(func == NULL) {
        LOG(CIRCLE_LOG_ERR, \
            "Attempted to create a queue with a null callback.");
        return -1;
    }

    jump[queue_id] = func;
    return 1;
}

int _CIRCLE_multi_exec(\
                       unsigned int queue_id, CIRCLE_handle* handle, CIRCLE_cb* jump)
{
    if(jump[queue_id] == NULL) {
        LOG(CIRCLE_LOG_ERR, \
            "Attempted to execute on a queue that doesn't exist (%d).", \
            queue_id);
        return -1;
    }

    jump[queue_id](handle);

    return 1;
}

/* EOF */
