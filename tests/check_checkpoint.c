#include <check.h>
#include <stdlib.h>
#include "libcircle.h"
#include "queue.h"

START_TEST
(test_checkpoint_single_read_write)
{
    fail();
}
END_TEST

START_TEST
(test_checkpoint_multiple_read_write)
{
    fail();
}
END_TEST

START_TEST
(test_checkpoint_empty_read_write)
{
    fail();
}
END_TEST
 
Suite *
check_checkpoint_suite (void)
{
    Suite *s = suite_create("check_checkpoint");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_checkpoint_single_read_write);
    tcase_add_test(tc_core, test_checkpoint_multiple_read_write);
    tcase_add_test(tc_core, test_checkpoint_empty_read_write);

    suite_add_tcase(s, tc_core);

    return s;
}

int
main (void)
{
    int number_failed;

    Suite *s = check_checkpoint_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* EOF */
