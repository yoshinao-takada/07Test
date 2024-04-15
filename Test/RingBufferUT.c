#include "BMRingBuffer.h"
#include <sys/param.h>
#define RB0SIZE 8
BMRingBuffer_SDECL(rb0, RB0SIZE);
#define RBCOUNT 4
BMRingBufferPool_SDECL(rbpool, RBCOUNT, RB0SIZE);

static const uint8_t TEST_DATA0[] = 
    "0123456789ABCDEFGIHJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz+-*/()[]\"'#!";

// check ringbuffer basic operations
BMStatus_t CheckRB0()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    uint8_t work[16];
    int idx = 0;
    do {
        if ((rb0.base.count != RB0SIZE) ||
            (rb0.base.wridx != 0) ||
            (rb0.base.rdidx != 0))
        {
            BMERR_LOGBREAKEX("rb0.base mismatch");
        }
        for (int i = 0; i < 8; i++)
        {
            int idx_save = idx;
            int idx_work = 0;
            for (; 1 == BMRingBuffer_Put(&rb0, TEST_DATA0 + idx); idx++);
            if (idx != (idx_save + RB0SIZE - 1))
            {
                BMERR_LOGBREAKEX("Fail in BMRingBuffer_Put()\n");
            }
            for (; 1 == BMRingBuffer_Get(&rb0, work + idx_work); idx_work++);
            if (strncmp(work, TEST_DATA0 + idx_save, RB0SIZE - 1))
            {
                BMERR_LOGBREAKEX("Fail in BMRingBuffer_Get()\n");
            }
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

BMStatus_t CheckRingBufferPool()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMRingBuffer_pt ringbuffers[RBCOUNT * 2];
    int i = 0;
    do {
        BMRingBufferPool_INIT(rbpool);
        for (; NULL != (ringbuffers[i] = BMRingBufferPool_Get(&rbpool)); i++);
        if (i != RBCOUNT)
        {
            BMERR_LOGBREAKEX("Fail in BMRingBufferPool_Get()");
        }
        for (i = 0; i < RBCOUNT; i++)
        {
            uint8_t* expected_address = rbpool_bufferbody + i * RB0SIZE;
            if (expected_address != ringbuffers[i]->bytes)
            { 
                BMERR_LOGBREAKEX(
                    "expected_address != ringbuffers[%d]->bytes", i);
            }
        }
        i = 0;
        for (; 
            BMSTATUS_SUCCESS == 
            BMRingBufferPool_Return(&rbpool, ringbuffers[i]); i++) ;
        if (i != RBCOUNT)
        {
            BMERR_LOGBREAKEX("Fail in BMRingBufferPool_Get()");
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

#define RBBUFFERSIZE 2 * MAX( \
    BMRINGBUFFERPOOL_LONGBUFFERCOUNT, \
    BMRINGBUFFERPOOL_SHORTBUFFERCOUNT)

BMStatus_t CheckGlobalRingBufferPool()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMRingBuffer_pt buffers[RBBUFFERSIZE];
    do {
        int i = 0;
        for (; 
            buffers[i] = 
            BMRingBufferPool_SGet(BMRingBufferPoolType_LONG); i++);
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

int RingBufferUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        BMRingBuffer_INIT(rb0);
        BMRingBufferPool_SInit(); // init the global ringbuffer pool
        if (BMSTATUS_SUCCESS != (status = CheckRB0()))
        {
            BMERR_LOGBREAKEX("Fail in CheckRB0()\n");
        }
        if (BMSTATUS_SUCCESS != (status = CheckRingBufferPool()))
        {
            BMERR_LOGBREAKEX("Fail in CheckRingBufferPool()");
        }
        if (BMSTATUS_SUCCESS != (status = CheckGlobalRingBufferPool()))
        {
            BMERR_LOGBREAKEX("Fail in CheckGlobalRingBufferPool()");
        }
        BMRingBufferPool_SDeinit(); // deinit the global ringbuffer pool
    } while (0);
    BMEND_FUNCEX(status);
    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}