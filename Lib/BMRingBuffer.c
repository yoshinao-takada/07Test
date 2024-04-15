#include "BMRingBuffer.h"

#pragma region BMRingBuffer_Impl
static uint16_t BMRingBuffer_Put_(BMRingBuffer_pt rb, const uint8_t* byte)
{
    uint16_t next_wridx = BMQBase_NextWrIdx(&rb->base);
    if (next_wridx == rb->base.rdidx)
    {
        return 0;
    }
    rb->bytes[rb->base.wridx] = *byte;
    rb->base.wridx = next_wridx;
    return 1;
}

static uint16_t BMRingBuffer_Get_(BMRingBuffer_pt rb, uint8_t* byte)
{
    uint16_t next_rdidx = BMQBase_NextRdIdx(&rb->base);
    if (rb->base.rdidx == rb->base.wridx)
    {
        return 0;
    }
    *byte = rb->bytes[rb->base.rdidx];
    rb->base.rdidx = next_rdidx;
    return 1;
}

uint16_t BMRingBuffer_Put(BMRingBuffer_pt rb, const uint8_t* byte)
{
    pthread_spin_lock(&rb->base.lock);
    uint16_t result = BMRingBuffer_Put_(rb, byte);
    pthread_spin_unlock(&rb->base.lock);
    return result;
}

uint16_t BMRingBuffer_Get(BMRingBuffer_pt rb, uint8_t* byte)
{
    pthread_spin_lock(&rb->base.lock);
    uint16_t result = BMRingBuffer_Get_(rb, byte);
    pthread_spin_unlock(&rb->base.lock);
    return result;
}

uint16_t BMRingBuffer_Puts
(BMRingBuffer_pt rb, const uint8_t* bytes, uint16_t count)
{
    uint16_t result = 0;
    pthread_spin_lock(&rb->base.lock);
    for (uint16_t i = 0; i < count; i++)
    {
        if (!BMRingBuffer_Put_(rb, bytes++))
        {
            break;
        }
        result++;
    }
    pthread_spin_unlock(&rb->base.lock);
    return result;
}

uint16_t BMRingBuffer_Gets
(BMRingBuffer_pt rb, uint8_t* bytes, uint16_t count)
{
    uint16_t result = 0;
    pthread_spin_lock(&rb->base.lock);
    for (uint16_t i = 0; i < count; i++)
    {
        if (!BMRingBuffer_Get_(rb, bytes++))
        {
            break;
        }
        result++;
    }
    pthread_spin_unlock(&rb->base.lock);
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
    BMStatus_t status = BMSTATUS_INVALID;
    pthread_spin_lock(&pool->lock);
    uint16_t ptr_offset = buffer - (pool->buffers);
    uint16_t clear_bit = (1 << ptr_offset);
    if ((ptr_offset < pool->size) && ((clear_bit & pool->used) != 0))
    {
        buffer->base.rdidx = buffer->base.wridx = 0;
        pool->used &= ~clear_bit;
        status = BMSTATUS_SUCCESS;
    }
    pthread_spin_unlock(&pool->lock);
    return status;
}

BMRingBufferPool_SDECL(LongRingBuffers,
    BMRINGBUFFERPOOL_LONGBUFFERCOUNT, BMRINGBUFFERPOOL_LONGBUFFERSIZE);
BMRingBufferPool_SDECL(ShortRingBuffers,
    BMRINGBUFFERPOOL_SHORTBUFFERCOUNT, BMRINGBUFFERPOOL_SHORTBUFFERSIZE);

BMRingBuffer_pt BMRingBufferPool_SGet(BMRingBufferPoolType_t type)
{
    return (type == BMRingBufferPoolType_SHORT) ?
        BMRingBufferPool_Get(&ShortRingBuffers) :
        BMRingBufferPool_Get(&LongRingBuffers);
}

BMStatus_t BMRingBufferPool_SReturn(BMRingBuffer_pt rb)
{
    return (rb->base.count == BMRINGBUFFERPOOL_SHORTBUFFERSIZE) ?
        BMRingBufferPool_Return(&ShortRingBuffers, rb) :
        (
            (rb->base.count == BMRINGBUFFERPOOL_LONGBUFFERSIZE) ?
                BMRingBufferPool_Return(&LongRingBuffers, rb):
                BMSTATUS_INVALID
        );
}
void BMRingBufferPool_SInit()
{
    BMRingBufferPool_INIT(ShortRingBuffers);
    BMRingBufferPool_INIT(LongRingBuffers);
}

void BMRingBufferPool_SDeinit()
{
    BMRingBufferPool_DEINIT(ShortRingBuffers);
    BMRingBufferPool_DEINIT(LongRingBuffers);
}

#pragma endregion BMRingBufferPool_Impl
