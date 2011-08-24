#include <check.h>
#include <stdlib.h>
#include "libcircle.h"
#include "queue.h"

START_TEST (test_queue_init_free)
{
    int free_result = -1;

    CIRCLE_queue_t *q;
    CIRCLE_init();

    q = CIRCLE_queue_init();
    fail_unless(q != NULL, "Initializing a queue failed.");

    free_result = CIRCLE_queue_free(q);
    fail_unless(free_result, "Circle context was not null after free.");
}
END_TEST

START_TEST (test_queue_pop_empty)
{
    int free_result = -1;
    char *result;

    CIRCLE_queue_t * queue;
    CIRCLE_init();

    queue = CIRCLE_queue_init();
    fail_unless(queue != NULL, "Initializing a queue failed.");

    result = CIRCLE_queue_pop(queue);
    free_unless(result == NULL, \
        "Something other than null was poped from an empty queue.");

    free_result = CIRCLE_queue_free(queue);
    fail_unless(free_result, "Circle context was not null after free.");
}
END_TEST

START_TEST (test_queue_single_push_pop)
{
    int free_result = -1;
    char * test_string = "Here's a test string!";
    char * pop_result;

    CIRCLE_queue_t * queue;
    CIRCLE_init();

    queue = CIRCLE_queue_init();
    fail_unless(queue != NULL, "Initializing a queue failed.");

    CIRCLE_queue_push(test_string);
    fail_unless(queue->size == 1, \
        "Queue size was not correct after a single push.");

    CIRCLE_queue_pop(queue, pop_result);
    fail_unless(queue->size == 1, \
        "Queue size was not correct after poping the last element.");

    fail_unless(strcmp(test_string, pop_result) == 0, \
        "Result poped from the queue does not match original.");

    free_result = CIRCLE_queue_free(queue);
    fail_unless(free_result, "Circle context was not null after free.");
}
END_TEST

START_TEST (test_queue_multiple_push_pop)
{
    int free_result = -1;
    char test_strings[] = {
        "first test string",
        "second test string",
        "third test string",
        "fourth test string",
        "fifth test string",
        "sixth test string",
        "seventh test string",
        "eighth test string",
        "nineth test string",
        "tenth test string"
    };

    CIRCLE_queue_t * queue;
    CIRCLE_init();

    queue = CIRCLE_queue_init();
    fail_unless(queue != NULL, "Initializing a queue failed.");

    /* FIXME: multiple push-pop here */

    free_result = CIRCLE_queue_free(queue);
    fail_unless(free_result, "Circle context was not null after free.");
}
END_TEST
 
Suite * check_queue_suite (void)
{
    Suite *s = suite_create("check_queue");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_queue_init_free);
    tcase_add_test(tc_core, test_queue_pop_empty);
    tcase_add_test(tc_core, test_queue_single_push_pop);

    suite_add_tcase(s, tc_core);

    return s;
}

int main (void)
{
    int number_failed;

    Suite *s = check_queue_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* EOF */
