#include "BMRingBuffer.h"

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

#pragma region BMRingBufferPool_Impl
BMRingBuffer_pt BMRingBufferPool_Get(BMRingBufferPool_pt pool)
{
    BMRingBuffer_pt p = NULL;
    pthread_spin_lock(&pool->lock);
    do {
        int16_t found = BMPoolSupport_FindAvailable(&pool->used, pool->size);
        if (found >= 0)
        {
            p = pool->buffers + found;
        }
    } while (0);
    pthread_spin_unlock(&pool->lock);
    return p;
}

BMStatus_t BMRingBufferPool_Return
(BMRingBufferPool_pt pool, BMRingBuffer_pt buffer)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    pthread_spin_lock(&pool->lock);
    uint16_t ptr_offset = buffer - (pool->buffers);
    if (ptr_offset < pool->size)
    {
        buffer->qbase.rdidx = buffer->qbase.wridx = 0;
        pool->used &= ~(1 << ptr_offset);
        status = BMSTATUS_SUCCESS;
    }
    pthread_spin_unlock(&pool->lock);
    return status;
}

BMRingBufferPool_SDECL(LongRingBuffer,
    BMRINGBUFFERPOOL_LONGBUFFERCOUNT, BMRINGBUFFERPOOL_LONGBUFFERSIZE);
BMRingBufferPool_SDECL(ShortRingBuffer,
    BMRINGBUFFERPOOL_SHORTBUFFERCOUNT, BMRINGBUFFERPOOL_SHORTBUFFERSIZE);

BMRingBuffer_pt BMRingBufferPool_SGet(BMRingBufferPoolType_t type)
{
    return (type == BMRingBufferPoolType_SHORT) ?
        BMRingBufferPool_Get(&ShortRingBuffer) :
        BMRingBufferPool_Get(&LongRingBuffer);
}

BMStatus_t BMRingBufferPool_SReturn(BMRingBuffer_pt rb)
{
    return (rb->qbase.count == BMRINGBUFFERPOOL_SHORTBUFFERSIZE) ?
        BMRingBufferPool_Return(&ShortRingBuffer, rb) :
        (
            (rb->qbase.count == BMRINGBUFFERPOOL_LONGBUFFERSIZE) ?
                BMRingBufferPool_Return(&LongRingBuffer, rb):
                BMSTATUS_INVALID
        );
}
void BMRingBufferPool_SInit();
void BMRingBufferPool_SDeinit();

#pragma endregion BMRingBufferPool_Impl
