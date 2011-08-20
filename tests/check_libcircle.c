#include <check.h>
#include "../src/libcircle.h"

START_TEST (test_context_create)
{
  CIRCLE_handle *c;
  c = CIRCLE_create();

  fail_unless (c != NULL, 
	       "Circle context was null.");

  CIRCLE_context_free(c);
}
END_TEST

int main (void)
{
    return 0;
}
