#include "../Lib/BMBuffer.h"
#include <assert.h>
#define POOL_SIZE       8
#define BUF_SIZE        64
BMBufferPool_SDECL(bufpool, POOL_SIZE, BUF_SIZE);

BMStatus_t CheckBufpool()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        // check pool size
        if (POOL_SIZE != bufpool.size)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "POOL_SIZE != bufpool.size");
        }
        // check buffer pointer
        if (bufpool.buffers != bufpool_buffers)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "bufpool.buffers != bufpool_buffers");
        }
        // check buffer body
        assert(bufpool.size == POOL_SIZE);
        assert(bufpool.used == 0);
        for (uint16_t i = 0; i < bufpool.size; i++)
        {
            if ((bufpool.buffers[i].size != BUF_SIZE) ||
                (bufpool.buffers[i].filled != 0))
            {
                status = BMSTATUS_INVALID;
                BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                    "bufpool.buffers[%d].size != BUF_SIZE", i);
            }
            if ((i == 0) && 
                (bufpool.buffers[i].buf != (uint8_t*)bufpool_bufferbody))
            {
                status = BMSTATUS_INVALID;
                BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                    "bufpool.buffers[0].buf != (uint8_t*)bufpool_bufferbody");
            }
            ptrdiff_t diff = bufpool.buffers[i].buf - bufpool.buffers[i-1].buf;
            if ((i) && (diff != BUF_SIZE))
            {
                status = BMSTATUS_INVALID;
                BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                    "diff != BUF_SIZE");
            }
        }
        if (status) break;
        // call lock and unlock
        BMBufferPool_LOCK(bufpool);
        BMBufferPool_UNLOCK(bufpool);
    } while (0);
    BMEND_FUNC(__FILE__, __FUNCTION__, __LINE__, status);
    return status;
}

// check buffer pool with get and return operations.
BMStatus_t GetAndReturn()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMBufferPool_INIT(bufpool);
    BMBuffer_pt buffers[POOL_SIZE + 1];
    do {
        int i = 0;
        for (; i < POOL_SIZE; i++)
        {
            buffers[i] = BMBufferPool_Get(&bufpool);
            if (!buffers[i])
            {
                status = BMSTATUS_INVALID;
                BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                    "Fail in buffers[%d] = BMBufferPool_Get(&)", i);
            }
        }
        if (status) break;
        buffers[i] = BMBufferPool_Get(&bufpool);
        if (buffers[i]) 
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in buffers[outside] = BMBufferPool_Get()");
        }
        if (bufpool.used != 0xff)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "bufpool.used != 0xff");
        }
        i = 0;
        BMBufferPool_Return(&bufpool, buffers[i++]);
        if (bufpool.used != 0xfe)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "bufpool.used != 0xfe");
        }
        for (; i < POOL_SIZE; i++)
        {
            BMBufferPool_Return(&bufpool, buffers[i]);
        }
        if (bufpool.used != 0)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "bufpool.used != 0");
        }
    } while (0);
    BMBufferPool_DEINIT(bufpool);
    BMEND_FUNC(__FILE__, __FUNCTION__, __LINE__, status);
    return status;
}

int BufferUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMBufferPool_INIT(bufpool);
    do {
        if (BMSTATUS_SUCCESS != (status = CheckBufpool()))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in CheckBufpool()");
        }
        if (BMSTATUS_SUCCESS != (status = GetAndReturn()))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in GetAndReturn()");
        }
    } while (0);
    BMBufferPool_DEINIT(bufpool);
    BMEND_FUNC(__FILE__, __FUNCTION__, __LINE__, status);
    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}