#ifndef SPSC_H
#define SPSC_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SPSC_CAP    1024u   /* must be a power of 2 */
#define SPSC_MASK   (SPSC_CAP - 1u)

typedef struct spsc {
    uint32_t      values[SPSC_CAP];
    atomic_size_t inPos;
    atomic_size_t outPos;
} spsc_t;

void spsc_init(spsc_t *q);
bool spsc_push(spsc_t *q, uint32_t val);
bool spsc_pop(spsc_t *q, uint32_t *out);
bool spsc_is_empty(spsc_t *q);
bool spsc_is_full(spsc_t *q);

#endif /* SPSC_H */