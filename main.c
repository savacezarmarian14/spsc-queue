#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "spsc_queue.h"

#define TEST_N  10000000u

static void *prodfunc(void *targs)
{
    spsc_t *q;
    uint32_t i;

    q = targs;
    for (i = 0; i < TEST_N; ++i) {
        while (!spsc_push(q, i))
            ;
    }
    return NULL;
}

static void *consfunc(void *targs)
{
    spsc_t *q;
    uint32_t i;
    uint32_t val;
    uintptr_t errors;

    q = targs;
    errors = 0;
    for (i = 0; i < TEST_N; ++i) {
        while (!spsc_pop(q, &val))
            ;
        if (val != i)
            errors++;
    }
    return (void *)errors;
}

int main(void)
{
    pthread_t prod;
    pthread_t cons;
    spsc_t *q;
    void *ret;
    uintptr_t errors;

    q = malloc(sizeof(*q));
    if (q == NULL)
        return EXIT_FAILURE;
    spsc_init(q);

    if (pthread_create(&prod, NULL, prodfunc, q) != 0) {
        free(q);
        return EXIT_FAILURE;
    }
    if (pthread_create(&cons, NULL, consfunc, q) != 0)
        return EXIT_FAILURE;   /* exit from main kills the spinning producer */

    pthread_join(prod, NULL);
    pthread_join(cons, &ret);
    errors = (uintptr_t)ret;

    if (errors == 0)
        printf("OK\n");
    else
        printf("FAIL (%zu errors)\n", (size_t)errors);

    free(q);
    return EXIT_SUCCESS;
}