#include <check.h>
#include "../src/libcircle.h"

START_TEST (test_context_create_free)
{
  CIRCLE_handle *h;

  h = CIRCLE_create();
  fail_unless (h != NULL, "Circle context was null after creation.");

  CIRCLE_free(c);
  fail_unless (h == NULL, "Circle context was null after free.");
}
END_TEST

int main (void)
{
    return 0;
}
