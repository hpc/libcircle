int printf (char *, ...);
int (i) = 1;
const int max = 500;
const int *p = &max;
int * const q = 0;

char foo[256];

extern double x;

float f = 123;
void *f_key = _make_global (sizeof (f), &f);

int g (void)
{
    static int i = 0;
    char *bar = foo;

    void (*x[5]) (int);

    return i++;
}

int (*f) (void) = g;

main ()
{
   int j;
   for (j = 0; j < 10; j++)
      printf ("%d\n", i + j + f ());
}
