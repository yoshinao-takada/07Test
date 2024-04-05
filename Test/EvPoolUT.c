#include "../Lib/BMBase.h"
#define QSIZE       4
#define POOLSIZE    4
BMEvQ_SDECL(evq, QSIZE);
BMEvPool_SDECL(evpl, POOLSIZE);

BMStatus_t CheckEvQ()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        if (evq.events != evq_ev)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("evq.events != evq_ev");
        }
        if (evq.qbase.count != QSIZE)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("evq.qbase.count != QSIZE");
        }
        if (evq.qbase.rdidx || evq.qbase.wridx)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("evq.qbase.rdidx || evq.qbase.wridx");
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

BMStatus_t CheckEvPool()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        if (evpl.ev != evpl_ev)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("evpl.ev != evpl_ev");
        }
        if (evpl.count != POOLSIZE)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("evpl.count != POOLSIZE");
        }
        if (evpl.used != 0)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("evpl.used != 0");
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

BMStatus_t GetReturnEvPool()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMEv_pt ev[POOLSIZE];
    do {
        ev[0] = BMEvPool_Get(&evpl);
        if ((ev[0] != evpl_ev) || (evpl.used != 1))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("(ev[0] != evpl_ev) || (evpl.used != 1)");
        }
        if (BMSTATUS_SUCCESS != (status = BMEvPool_Return(&evpl, ev[0])))
        {
            BMERR_LOGBREAKEX("BMEvPool_Return(&evpl, ev)");
        }
        int i = 0;
        for (; i < evpl.count; i++)
        {
            ev[i] = BMEvPool_Get(&evpl);
            if (!ev[i])
            {
                status = BMSTATUS_INVALID;
                BMERR_LOGBREAKEX("!ev[i]");
            }
        }
        if (status) break;
        if (NULL != BMEvPool_Get(&evpl))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("NULL != BMEvPool_Get(&evpl)");
        }
        i = 0;
        for (; i < evpl.count; i++)
        {
            if (BMSTATUS_SUCCESS != (status = BMEvPool_Return(&evpl, ev[i])))
            {
                BMERR_LOGBREAKEX("BMEvPool_Return(, ev[%d] ", i);
            }
        }
        if (status) break;
        if (evpl.used)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("evpl.used != 0 ");
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

BMStatus_t GetEvPoolPutEvQ()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMEv_pt ev[QSIZE];
    int i = 0;
    do {
        for (; i < ARRAYSIZE(ev); i++)
        {
            ev[i] = BMEvPool_Get(&evpl);
        }
        i = 0;
        for (; i < QSIZE; i++)
        {
            if (0 == BMEvQ_Put(&evq, ev[i])) break;
        }
        if (i != (QSIZE - 1))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("i != (QSIZE - 1), i = %d ", i);
        }
        for (i = 0; i < (QSIZE - 1); i++)
        {
            BMEv_pt evtemp = NULL;
            uint16_t count = BMEvQ_Get(&evq, &evtemp);
            if (count != 1 || evtemp->listeners != 1)
            {
                status = BMSTATUS_INVALID;
                BMERR_LOGBREAKEX("count != 1 || evtemp->listeners != 1 ");
            }
            evtemp->listeners--;
        }
        i = 0;
        for (; i < QSIZE; i++)
        {
            if (0 == BMEvQ_Put(&evq, ev[i])) break;
        }
        if (i != (QSIZE - 1))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("i != (QSIZE - 1), i = %d ", i);
        }
        for (i = 0; i < (QSIZE - 1); i++)
        {
            BMEv_pt evtemp = NULL;
            uint16_t count = BMEvQ_Get(&evq, &evtemp);
            if (count != 1 || evtemp->listeners != 1)
            {
                status = BMSTATUS_INVALID;
                BMERR_LOGBREAKEX("count != 1 || evtemp->listeners != 1 ");
            }
            evtemp->listeners--;
        }
        for (i = 0; i < ARRAYSIZE(ev); i++)
        {
            BMEvPool_Return(&evpl, ev[i]);
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

int EvPoolUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMEvQ_INIT(evq);
    BMEvPool_INIT(evpl);
    do {
        if (BMSTATUS_SUCCESS != (status = CheckEvQ()))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "CheckEvQ() failed");
        }
        if (BMSTATUS_SUCCESS != (status = CheckEvPool()))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "CheckEvPool() failed");
        }
        if (BMSTATUS_SUCCESS != (status = GetReturnEvPool()))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "GetReturnPool() failed");
        }
        if (BMSTATUS_SUCCESS != (status = GetEvPoolPutEvQ()))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "GetPoolPutQ() failed");
        }
    } while (0);
    BMEvQ_DEINIT(evq);
    BMEvPool_DEINIT(evpl);
    BMEND_FUNCEX(status);
    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}