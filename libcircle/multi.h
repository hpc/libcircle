#ifndef MULTI_H
#define MULTI_H

int CIRCLE_multi_add_start(unsigned int queue_id, CIRCLE_cb func);
int CIRCLE_multi_add_process(unsigned int queue_id, CIRCLE_cb func);
int CIRCLE_multi_exec_start(unsigned int queue_id, CIRCLE_handle* handle);
int CIRCLE_multi_exec_process(unsigned int queue_id, CIRCLE_handle* handle);

int _CIRCLE_multi_add(unsigned int queue_id, CIRCLE_cb func, CIRCLE_cb* jump);
int _CIRCLE_multi_exec(unsigned int queue_id, CIRCLE_handle* handle, \
                       CIRCLE_cb* jump);

#endif /* MULTI_H */
