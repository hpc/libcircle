#include <libcircle.h>

CIRCLE_cb CIRCLE_multi_jump_table[CIRCLE_MAX_QUEUE_COUNT];

int CIRCLE_multi_queue_create(unsigned int queue_id, CIRCLE_cb func)
{
    if(func == NULL)
	{
        LOG(CIRCLE_LOG_ERR, "Attempted to create a queue with a null callback.");
		return -1;
	}

    CIRCLE_multi_jump_table[queue_id] = func;
	return 1;
}

int CIRCLE_multi_exec(unsigned int queue_id, CIRCLE_handle* handle)
{
    if(CIRCLE_multi_jump_table[queue_id] == NULL)
	{
        LOG(CIRCLE_LOG_ERR, "Attempted to execute on a queue that doesn't exist (%d).", queue_id);
		return -1;
	}

    CIRCLE_multi_jump_table[queue_id](handle);

	return 1;
}

/* EOF */
