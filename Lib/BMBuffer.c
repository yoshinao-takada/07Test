#include "BMBuffer.h"
#pragma region BMBufferQ_Impl
uint16_t BMBufferQ_Put(BMBufferQ_pt q, BMBuffer_pt buffer)
{
    uint16_t result = 0;
    BMQBase_LOCK(q->qbase);
    uint16_t next_wridx = (q->qbase.wridx + 1);
    if (next_wridx >= q->qbase.count)
    {
        next_wridx -= q->qbase.count;
    }
    if (next_wridx != q->qbase.rdidx)
    {
        q->buffers[q->qbase.wridx] = buffer;
        q->qbase.wridx = next_wridx;
        result++;
    }
    BMQBase_UNLOCK(q->qbase);
    return result;
}

uint16_t BMBufferQ_Get(BMBufferQ_pt q, BMBuffer_pt *ppbuffer)
{
    uint16_t result = 0;
    BMQBase_LOCK(q->qbase);
    if (q->qbase.rdidx != q->qbase.wridx)
    {
        *ppbuffer = q->buffers[q->qbase.rdidx];
        q->qbase.rdidx++;
        if (q->qbase.rdidx >= q->qbase.count)
        {
            q->qbase.rdidx -= q->qbase.count;
        }
        result++;
    }
    BMQBase_UNLOCK(q->qbase);
    return result;
}
#pragma endregion BMBufferQ_Impl

#pragma region BMBufferPool_Impl
static int FindAvailable(uint16_t *used, uint16_t size)
{
    int found = -1;
    uint16_t flag = 1;
    for (int i = 0; i < size; i++)
    {
        if (0 == (*used & flag))
        {
            *used |= flag;
            found = i;
            break;
        }
        else
        {
            flag <<= 1;
        }
    }
    return found;
}

BMBuffer_pt BMBufferPool_Get(BMBufferPool_pt bpl)
{
    BMBuffer_pt p = NULL;
    pthread_spin_lock(&bpl->lock);
    do {
        int found = FindAvailable(&bpl->used, bpl->size);
        if (found >= 0)
        {
            p = bpl->buffers + found;
        }
    } while (0);
    pthread_spin_unlock(&bpl->lock);
    return p;
}

BMStatus_t BMBufferPool_Return(BMBufferPool_pt bpl, BMBuffer_cpt buffer)
{
    BMStatus_t result = BMSTATUS_INVALID;
    pthread_spin_lock(&bpl->lock);
    uint16_t ptr_offset = buffer - (bpl->buffers);
    if (ptr_offset < bpl->size)
    {
        bpl->used &= ~(1 << ptr_offset);
        result = BMSTATUS_SUCCESS;
    }
    pthread_spin_unlock(&bpl->lock);
    return result;
}
#pragma endregion BMBufferPool_Impl