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

BMBuffer_pt BMBufferQ_Peek(BMBufferQ_pt q)
{
    if (q->qbase.rdidx == q->qbase.wridx)
    { // queue is empty.
        return NULL;
    }
    return q->buffers[q->qbase.rdidx];
}
#pragma endregion BMBufferQ_Impl

#pragma region BMBufferPool_Impl

BMBuffer_pt BMBufferPool_Get(BMBufferPool_pt bpl)
{
    BMBuffer_pt p = NULL;
    pthread_spin_lock(&bpl->lock);
    do {
        int found = BMPoolSupport_FindAvailable(&bpl->used, bpl->size);
        if (found >= 0)
        {
            p = bpl->buffers + found;
        }
    } while (0);
    pthread_spin_unlock(&bpl->lock);
    return p;
}

BMStatus_t BMBufferPool_Return(BMBufferPool_pt pool, BMBuffer_pt buffer)
{
    BMStatus_t status = BMSTATUS_INVALID;
    pthread_spin_lock(&pool->lock);
    uint16_t ptr_offset = buffer - (pool->buffers);
    uint16_t clear_bit = (1 << ptr_offset);
    if ((ptr_offset < pool->size) && ((clear_bit & pool->used) != 0))
    {
        buffer->filled = buffer->crunched = 0;
        pool->used &= ~clear_bit;
        status = BMSTATUS_SUCCESS;
    }
    pthread_spin_unlock(&pool->lock);
    return status;
}
#pragma endregion BMBufferPool_Impl

#pragma region BufferPool_GlobalInstaces
BMBufferPool_SDECL(ShortBufferPool,
    BMBUFFERPOOL_SHORTBUFFERCOUNT, BMBUFFERPOOL_SHORTBUFFERSIZE);
BMBufferPool_SDECL(LongBufferPool,
    BMBUFFERPOOL_LONGBUFFERCOUNT, BMBUFFERPOOL_LONGBUFFERSIZE);

void BMBufferPool_SInit()
{
    BMBufferPool_INIT(ShortBufferPool);
    BMBufferPool_INIT(LongBufferPool);
}
void BMBufferPool_SDeinit()
{
    BMBufferPool_DEINIT(ShortBufferPool);
    BMBufferPool_DEINIT(LongBufferPool);
}
BMBuffer_pt BMBufferPool_SGet(BMBufferPoolType_t type)
{
    return (type == BMBufferPoolType_LONG) ? 
        BMBufferPool_Get(&LongBufferPool) : BMBufferPool_Get(&ShortBufferPool);
}

BMStatus_t BMBufferPool_SReturn(BMBuffer_pt buffer)
{
    return (buffer->size == BMBUFFERPOOL_LONGBUFFERSIZE) ?
        BMBufferPool_Return(&LongBufferPool, buffer) :
        (
            (buffer->size == BMBUFFERPOOL_SHORTBUFFERSIZE) ?
                BMBufferPool_Return(&ShortBufferPool, buffer):
                BMSTATUS_INVALID
        );
}

#pragma endregion BufferPool_GlobalInstaces
