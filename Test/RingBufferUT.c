#include "BMRingBuffer.h"
#define RB0SIZE 8
BMRingBuffer_SDECL(rb0, RB0SIZE);

static const uint8_t TEST_DATA0[] = "0123456789ABCDEFGIHJKLMNO";

// 
BMStatus_t CheckRB0()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    int i;
    uint8_t work[16];
    do {
        if ((rb0.qbase.count != RB0SIZE) ||
            (rb0.qbase.wridx != 0) ||
            (rb0.qbase.rdidx != 0))
        {
            BMERR_LOGBREAKEX("rb0.qbase mismatch");
        }
        i = 0;
        for (; 1 != BMRingBuffer_Put(&rb0, TEST_DATA0 + i); i++) ;
        if (i != RB0SIZE)
        {
            BMERR_LOGBREAKEX("Fail in BMRingBuffer_Put()");
        }
        i = 0;
        for (; 1 != BMRingBuffer_Get(&rb0, work + i); i++) ;
        if ((i != RB0SIZE) || strncmp(work, TEST_DATA0, RB0SIZE-1))
        {
            BMERR_LOGBREAKEX("Fail in BMRingBuffer_Get()");
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

int RingBufferUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        BMRingBuffer_INIT(rb0);

    } while (0);
    BMEND_FUNCEX(status);
    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}