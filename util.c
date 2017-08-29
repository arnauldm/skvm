#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

void pexit (char *s)
{
    perror (s);
    exit (1);
}

