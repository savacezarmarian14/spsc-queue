#include "spsc_queue.h"

void spsc_init(spsc_t *q)
{
    atomic_store_explicit(&q->inPos, 0, memory_order_relaxed);
    atomic_store_explicit(&q->outPos, 0, memory_order_relaxed);
}

bool spsc_push(spsc_t *q, uint32_t val)
{
    if (spsc_is_full(q))
        return false;
    
    q->values[atomic_load_explicit(&q->inPos, memory_order_relaxed) & SPSC_MASK] = val;
    atomic_fetch_add_explicit(&q->inPos, 1, memory_order_release);
    return true;
}

bool spsc_pop(spsc_t *q, uint32_t *out) {
    if (spsc_is_empty(q))
        return false;
    *out = q->values[atomic_load_explicit(&q->outPos, memory_order_relaxed) & SPSC_MASK];
    atomic_fetch_add_explicit(&q->outPos, 1, memory_order_release);
    return true;
}

bool spsc_is_empty(spsc_t *q)
{
    if (atomic_load_explicit(&q->inPos, memory_order_acquire) - 
        atomic_load_explicit(&q->outPos, memory_order_relaxed) == 0)
        return true;
    return false;
}

bool spsc_is_full(spsc_t *q)
{
    if (atomic_load_explicit(&q->inPos, memory_order_relaxed) - 
        atomic_load_explicit(&q->outPos, memory_order_acquire) == SPSC_CAP)
        return true;
    return false;
}
