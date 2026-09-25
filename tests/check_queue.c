#include <check.h>
#include <stdlib.h>
#include "libcircle.h"
#include "queue.h"

START_TEST
(test_queue_init_free)
{
    int free_result = -1;

    CIRCLE_internal_queue_t* q;
    CIRCLE_init(0, NULL, CIRCLE_DEFAULT_FLAGS);

    q = CIRCLE_internal_queue_init();
    /* the queue must initialize */
    ck_assert_ptr_ne(q, NULL);

    free_result = CIRCLE_internal_queue_free(q);
    /* freeing the queue must succeed */
    ck_assert_int_ge(free_result, 0);

    CIRCLE_finalize();
}
END_TEST

START_TEST
(test_queue_pop_empty)
{
    int free_result = -1;
    char result[CIRCLE_MAX_STRING_LEN];

    memset(&result, 0, sizeof(char) * CIRCLE_MAX_STRING_LEN);

    CIRCLE_internal_queue_t* q;
    CIRCLE_init(0, NULL, CIRCLE_DEFAULT_FLAGS);

    q = CIRCLE_internal_queue_init();
    /* the queue must initialize */
    ck_assert_ptr_ne(q, NULL);

    CIRCLE_internal_queue_pop(q, result);
    /* an empty queue must pop nothing */
    ck_assert_uint_eq(strlen(result), 0);

    free_result = CIRCLE_internal_queue_free(q);
    /* freeing the queue must succeed */
    ck_assert_int_ne(free_result, 0);

    CIRCLE_finalize();
}
END_TEST

START_TEST
(test_queue_single_push_pop)
{
    int free_result = -1;
    char* test_string = "Here's a test string!";
    char result[CIRCLE_MAX_STRING_LEN];

    CIRCLE_internal_queue_t* q;
    CIRCLE_init(0, NULL, CIRCLE_DEFAULT_FLAGS);

    q = CIRCLE_internal_queue_init();
    /* the queue must initialize */
    ck_assert_ptr_ne(q, NULL);

    CIRCLE_internal_queue_push(q, test_string);
    /* a single push must leave one item queued */
    ck_assert_int_eq(q->count, 1);

    CIRCLE_internal_queue_pop(q, result);
    /* popping the last element must empty the queue */
    ck_assert_int_eq(q->count, 0);

    /* what comes out must match what went in */
    ck_assert_str_eq(test_string, result);

    free_result = CIRCLE_internal_queue_free(q);
    /* freeing the queue must succeed */
    ck_assert_int_ne(free_result, 0);

    CIRCLE_finalize();
}
END_TEST

START_TEST
(test_queue_multiple_push_pop)
{
    int free_result = -1;
    char result[CIRCLE_MAX_STRING_LEN];

    char** test_strings = (char**) malloc(sizeof(char*) * 10);
    test_strings[0] = "first test string";
    test_strings[1] = "second test string";
    test_strings[2] = "third test string";
    test_strings[3] = "fourth test string";
    test_strings[4] = "fifth test string";
    test_strings[5] = "sixth test string";
    test_strings[6] = "seventh test string";
    test_strings[7] = "eighth test string";
    test_strings[8] = "nineth test string";
    test_strings[9] = "tenth test string";

    CIRCLE_internal_queue_t* q;
    CIRCLE_init(0, NULL, CIRCLE_DEFAULT_FLAGS);

    q = CIRCLE_internal_queue_init();
    /* the queue must initialize */
    ck_assert_ptr_ne(q, NULL);

    /* Warm it up a bit */
    CIRCLE_internal_queue_push(q, test_strings[0]);
    CIRCLE_internal_queue_pop(q, result);
    CIRCLE_internal_queue_push(q, test_strings[1]);
    CIRCLE_internal_queue_pop(q, result);

    /* the pop must return the expected string */
    ck_assert_str_eq(test_strings[1], result);

    /* two pushes and two pops must leave the queue empty */
    ck_assert_int_eq(q->count, 0);

    /* Now lets try multiple ones */
    CIRCLE_internal_queue_push(q, test_strings[2]);
    CIRCLE_internal_queue_push(q, test_strings[3]);
    CIRCLE_internal_queue_push(q, test_strings[4]);
    CIRCLE_internal_queue_push(q, test_strings[5]);
    CIRCLE_internal_queue_push(q, test_strings[6]);
    CIRCLE_internal_queue_push(q, test_strings[7]); // count = 6
    CIRCLE_internal_queue_pop(q, result);
    CIRCLE_internal_queue_pop(q, result);
    CIRCLE_internal_queue_pop(q, result);
    CIRCLE_internal_queue_pop(q, result); // count = 2
    CIRCLE_internal_queue_push(q, test_strings[8]);
    CIRCLE_internal_queue_push(q, test_strings[9]);
    CIRCLE_internal_queue_push(q, test_strings[0]); // count = 5
    CIRCLE_internal_queue_pop(q, result);
    CIRCLE_internal_queue_pop(q, result);
    CIRCLE_internal_queue_pop(q, result);
    CIRCLE_internal_queue_pop(q, result);
    CIRCLE_internal_queue_pop(q, result); // count = 0

    /* the pop must return the expected string */
    ck_assert_str_eq(test_strings[2], result);

    /* the sequence above must leave the queue empty */
    ck_assert_int_eq(q->count, 0);

    /* Lets just try a few randomly */
    CIRCLE_internal_queue_push(q, test_strings[1]);
    CIRCLE_internal_queue_pop(q, result);
    CIRCLE_internal_queue_pop(q, result); // count = 0
    CIRCLE_internal_queue_push(q, test_strings[2]);
    CIRCLE_internal_queue_pop(q, result);
    CIRCLE_internal_queue_push(q, test_strings[3]);
    CIRCLE_internal_queue_push(q, test_strings[4]);
    CIRCLE_internal_queue_push(q, test_strings[5]); // count = 3
    CIRCLE_internal_queue_pop(q, result); // count = 2

    /* the pop must return the expected string */
    ck_assert_str_eq(test_strings[5], result);

    /* the sequence above must leave two items queued */
    ck_assert_int_eq(q->count, 2);

    free_result = CIRCLE_internal_queue_free(q);
    /* freeing the queue must succeed */
    ck_assert_int_ne(free_result, 0);

    CIRCLE_finalize();
}
END_TEST

Suite*
check_queue_suite(void)
{
    Suite* s = suite_create("check_queue");
    TCase* tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_queue_init_free);
    tcase_add_test(tc_core, test_queue_pop_empty);
    tcase_add_test(tc_core, test_queue_single_push_pop);
    tcase_add_test(tc_core, test_queue_multiple_push_pop);

    suite_add_tcase(s, tc_core);

    return s;
}

int
main(void)
{
    int number_failed;

    Suite* s = check_queue_suite();
    SRunner* sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* EOF */
