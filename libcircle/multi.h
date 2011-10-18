#ifndef MULTI_H
#define MULTI_H

int CIRCLE_multi_queue_create(unsigned int queue_id, CIRCLE_cb func);
int CIRCLE_multi_exec(unsigned int queue_id, CIRCLE_handle* handle);

#endif /* MULTI_H */
