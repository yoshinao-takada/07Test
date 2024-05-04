#define BMBASE_C
#include "BMBase.h"
// return the bit number of the 1st zero searching from LSB.
// The bit in x is set if the bit is zero.
int16_t BMPoolSupport_FindAvailable(uint16_t *x, uint16_t count)
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
uint16_t BMQBase_NextWrIdx(BMQBase_pt q)
{
    uint16_t next_wridx = q->wridx + 1;
    if (next_wridx == q->count)
    {
        next_wridx -= q->count;
    }
    return next_wridx;
}

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
        evpool->ev[i].id = 0;
        evpool->ev[i].listeners = 0;
        evpool->ev[i].param = NULL;
    }
}

BMEv_pt BMEvPool_Get(BMEvPool_pt evpool)
{
    BMEv_pt result = NULL;
    pthread_spin_lock(&evpool->lock);
    do {
        int16_t bitnum = BMPoolSupport_FindAvailable(&evpool->used, evpool->count);
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
    evpool->ev[offset].param = NULL;
    evpool->ev[offset].id = 
    evpool->ev[offset].listeners = 0;
    pthread_spin_unlock(&evpool->lock);
    return BMSTATUS_SUCCESS;
}

BMEvQ_SADECL(sevqpool, BMEVQPOOL_QSIZE, BMEVQPOOL_QCOUINT);
static uint16_t sevqpool_used = 0;

void BMEvQPool_SInit() 
{
    BMEvQ_AINIT(sevqpool);
}

void BMEvQPool_SDeinit()
{
    BMEvQ_ADEINIT(sevqpool);
}

BMEvQ_pt BMEvQPool_SGet()
{
    uint16_t qcount = ARRAYSIZE(sevqpool);
    int16_t idx = BMPoolSupport_FindAvailable(&sevqpool_used, qcount);
    return (idx < 0) ? NULL : sevqpool + idx;
}

BMStatus_t BMEvQPool_SReturn(BMEvQ_pt q)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    ptrdiff_t offset = q - sevqpool;
    uint16_t offset_bit = (1 << offset);
    uint16_t qcount = ARRAYSIZE(sevqpool);
    if (((uint16_t)offset < qcount) && 
        ((sevqpool_used & offset_bit) != 0))
    {
        sevqpool_used &= ~(1 << offset);
    }
    else
    {
        status = BMSTATUS_INVALID;
    }
    return status;
}
#pragma endregion BMEvPool_Impl
