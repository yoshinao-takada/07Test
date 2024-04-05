#include "BMBase.h"
// return the bit number of the 1st zero searching from LSB.
// The bit in x is set if the bit is zero.
static int16_t Get1stZero(uint16_t *x, uint16_t count)
{
    uint16_t mask = 1;
    int16_t bitnum = -1;
    for (int16_t i = 0; i < count; i++, mask <<= 1)
    {
        if (!(mask & *x))
        {
            bitnum = i;
            *x |= mask;
            break;
        }
    }
    return bitnum;
}

#pragma region BMQBase_Impl
static
uint16_t BMQBase_NextWrIdx(BMQBase_pt q)
{
    uint16_t next_wridx = q->wridx + 1;
    if (next_wridx == q->count)
    {
        next_wridx -= q->count;
    }
    return next_wridx;
}

static
uint16_t BMQBase_NextRdIdx(BMQBase_pt q)
{
    uint16_t next_rdidx = q->rdidx + 1;
    if (next_rdidx == q->count)
    {
        next_rdidx -= q->count;
    }
    return next_rdidx;
}
#pragma endregion BMQBase_Impl

#pragma region BMEvQ_Impl
uint16_t BMEvQ_Put(BMEvQ_pt queue, BMEv_pt pev)
{
    pthread_spin_lock(&queue->qbase.lock);
    uint16_t next_wridx = BMQBase_NextWrIdx(&queue->qbase);
    if (next_wridx == queue->qbase.rdidx)
    {
        pthread_spin_unlock(&queue->qbase.lock);
        return 0;
    }
    pev->listeners++;
    queue->events[queue->qbase.wridx] = pev;
    queue->qbase.wridx = next_wridx;
    pthread_spin_unlock(&queue->qbase.lock);
    return 1;
}

uint16_t BMEvQ_Get(BMEvQ_pt queue, BMEv_pt *ppev)
{
    pthread_spin_lock(&queue->qbase.lock);
    if (queue->qbase.rdidx == queue->qbase.wridx)
    {
        pthread_spin_unlock(&queue->qbase.lock);
        return 0;
    }
    *ppev = queue->events[queue->qbase.rdidx];
    queue->qbase.rdidx = BMQBase_NextRdIdx(&queue->qbase);
    pthread_spin_unlock(&queue->qbase.lock);
    return 1;
}

void BMEvQ_Init(BMEvQ_pt queue)
{
    pthread_spin_init(&queue->qbase.lock, PTHREAD_PROCESS_PRIVATE);
}

void BMEvQ_Deinit(BMEvQ_pt queue)
{
    pthread_spin_destroy(&queue->qbase.lock);
}
#pragma endregion BMEvQ_Impl

#pragma region BMEvPool_Impl

void BMEvPool_Init(BMEvPool_pt evpool)
{
    for (uint16_t i = 0; i < evpool->count; i++)
    {
        evpool->ev[i].id.ptr = NULL;
        evpool->ev[i].listeners = 0;
    }
}

BMEv_pt BMEvPool_Get(BMEvPool_pt evpool)
{
    BMEv_pt result = NULL;
    pthread_spin_lock(&evpool->lock);
    do {
        int16_t bitnum = Get1stZero(&evpool->used, evpool->count);
        if (bitnum < 0) break;
        result = evpool->ev + bitnum;
    } while (0);    
    pthread_spin_unlock(&evpool->lock);
    return result;
}

BMStatus_t BMEvPool_Return(BMEvPool_pt evpool, BMEv_pt ev)
{
    pthread_spin_lock(&evpool->lock);
    uint16_t offset = ev - (evpool->ev);
    uint16_t flag = (1 << offset);
    if ((flag & evpool->used) == 0)
    {
        return BMSTATUS_INVALID;
    }
    evpool->used &= ~flag;
    evpool->ev[offset].id.ptr = NULL;
    evpool->ev[offset].listeners = 0;
    pthread_spin_unlock(&evpool->lock);
    return BMSTATUS_SUCCESS;
}

#pragma endregion BMEvPool_Impl

#pragma region BMRingBuffer_Impl
static uint16_t BMRingBuffer_Put_(BMRingBuffer_pt rb, const uint8_t* byte)
{
    uint16_t next_wridx = BMQBase_NextWrIdx(&rb->qbase);
    if (next_wridx == rb->qbase.rdidx)
    {
        return 0;
    }
    rb->bytes[rb->qbase.wridx] = *byte;
    rb->qbase.wridx = next_wridx;
    return 1;
}

static uint16_t BMRingBuffer_Get_(BMRingBuffer_pt rb, uint8_t* byte)
{
    uint16_t next_rdidx = BMQBase_NextRdIdx(&rb->qbase);
    if (rb->qbase.rdidx == rb->qbase.wridx)
    {
        return 0;
    }
    *byte = rb->bytes[rb->qbase.rdidx];
    rb->qbase.rdidx = next_rdidx;
    return 1;
}

uint16_t BMRingBuffer_Put(BMRingBuffer_pt rb, const uint8_t* byte)
{
    pthread_spin_lock(&rb->qbase.lock);
    uint16_t result = BMRingBuffer_Put_(rb, byte);
    pthread_spin_unlock(&rb->qbase.lock);
    return result;
}

uint16_t BMRingBuffer_Get(BMRingBuffer_pt rb, uint8_t* byte)
{
    pthread_spin_lock(&rb->qbase.lock);
    uint16_t result = BMRingBuffer_Get_(rb, byte);
    pthread_spin_unlock(&rb->qbase.lock);
    return result;
}

uint16_t BMRingBuffer_Puts
(BMRingBuffer_pt rb, const uint8_t* bytes, uint16_t count)
{
    uint16_t result = 0;
    pthread_spin_lock(&rb->qbase.lock);
    for (uint16_t i = 0; i < count; i++)
    {
        if (!BMRingBuffer_Put_(rb, bytes++))
        {
            break;
        }
        result++;
    }
    pthread_spin_unlock(&rb->qbase.lock);
    return result;
}

uint16_t BMRingBuffer_Gets
(BMRingBuffer_pt rb, uint8_t* bytes, uint16_t count)
{
    uint16_t result = 0;
    pthread_spin_lock(&rb->qbase.lock);
    for (uint16_t i = 0; i < count; i++)
    {
        if (!BMRingBuffer_Get_(rb, bytes++))
        {
            break;
        }
        result++;
    }
    pthread_spin_unlock(&rb->qbase.lock);
    return result;
}
#pragma endregion BMRingBuffer_Impl