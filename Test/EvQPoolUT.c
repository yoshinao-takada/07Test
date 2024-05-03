#include "BMBase.h"
static BMStatus_t EvQPool_CheckQIntegrity(BMEvQ_pt q)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        if (q->qbase.count != BMEVQPOOL_QSIZE)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("q->qbase.count != BMEVQPOOL_QSIZE");
        }
        if (q->qbase.rdidx != 0 || q->qbase.wridx != 0)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("(q->qbase.rdidx != 0 || q->qbase.wridx != 0)");
        }
        if (pthread_spin_trylock(&q->qbase.lock))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("q->qbase.lock not initialized.");
        }
        if (EBUSY != pthread_spin_trylock(&q->qbase.lock))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("q->qbase.lock not initialized.");
        }
        pthread_spin_unlock(&q->qbase.lock);
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}
static BMStatus_t EvQPool_GetReturnUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMEvQ_pt queues[BMEVQPOOL_QCOUINT * 2];
    do {
        int i = 0;
        #pragma region CHECK_POOL_OPERATION
        for (; queues[i] = BMEvQPool_SGet(); i++) ;

        if (i != BMEVQPOOL_QCOUINT)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in BMEvQPool_SGet()");
        }       

        for (i = 0;
             BMSTATUS_SUCCESS == BMEvQPool_SReturn(queues[i]);
             i++) ;

        if (i != BMEVQPOOL_QCOUINT)
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in BMEvQPool_SReturn()");
        }
        #pragma endregion CHECK_POOL_OPERATION

        queues[0] = BMEvQPool_SGet();
        if (BMSTATUS_SUCCESS != (status = EvQPool_CheckQIntegrity(queues[0])))
        {
            BMERR_LOGBREAKEX("Fail in EvQPool_CheckQIntegrity()");
        }
        if (BMSTATUS_SUCCESS != (status = BMEvQPool_SReturn(queues[0])))
        {
            BMERR_LOGBREAKEX("Fail in BMEvQPool_SReturn()");
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

static BMEv_t ev[] = {
    { 0, 0, (void*)0 },
    { 1, 0, (void*)2 },
    { 2, 0, (void*)4 },
    { 3, 0, (void*)6 },
    { 4, 0, (void*)8 },
    { 5, 0, (void*)10 },
    { 6, 0, (void*)12 },
    { 7, 0, (void*)14 },
};

static BMStatus_t EvQPool_UseEvQUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMEvQ_pt q = BMEvQPool_SGet();
    BMEvQ_pt q2 = BMEvQPool_SGet();
    BMEv_pt evgotten;
    do {
        int i;
        for (i = 0; 1 == BMEvQ_Put(q, ev + i); i++) ;
        if (i != (BMEVQPOOL_QSIZE - 1))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in BMEvQ_Put(q, ev + i)");
        }
        for (int j = 0; j < i; j++)
        {
            if (ev[j].listeners != 1)
            {
                status = BMSTATUS_INVALID;
                BMERR_LOGBREAKEX("(ev[j].listeners != 1)");
            }
        }
        if (status) break;
        for (i = 0; 1 == BMEvQ_Put(q2, ev + i); i++) ;
        if (i != (BMEVQPOOL_QSIZE - 1))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in BMEvQ_Put(q, ev + i)");
        }
        for (int j = 0; j < i; j++)
        {
            if (ev[j].listeners != 2)
            {
                status = BMSTATUS_INVALID;
                BMERR_LOGBREAKEX("(ev[j].listeners != 1)");
            }
        }
        for (i = 0; 1 == BMEvQ_Get(q, &evgotten); i++) 
        {
            evgotten->listeners --;
        }
        for (i = 0; i < q2->qbase.count - 1; i++)
        {
            if (1 != ev[i].listeners)
            {
                status = BMSTATUS_INVALID;
                BMERR_LOGBREAKEX("(1 != ev[i].listeners)");
            }
        }
        if (status) break;
        for (i = 0; 1 == BMEvQ_Get(q2, &evgotten); i++) 
        {
            evgotten->listeners --;
            if (evgotten->listeners)
            {
                status = BMSTATUS_INVALID;
                BMERR_LOGBREAKEX("evgotten->listeners != 0");
            }
        }
    } while (0);
    BMEvQPool_SReturn(q);
    BMEvQPool_SReturn(q2);
    BMEND_FUNCEX(status);
    return status;
}

int EvQPoolUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMEvQPool_SInit();
    do {
        if (BMSTATUS_SUCCESS != (status = EvQPool_GetReturnUT()))
        {
            BMERR_LOGBREAKEX("Fail in EvQPool_GetReturnUT()");
        }
        if (BMSTATUS_SUCCESS != (status = EvQPool_UseEvQUT()))
        {
            BMERR_LOGBREAKEX("Fail in EvQPool_UseEvQUT()");
        }
    } while (0);
    BMEvQPool_SDeinit();
    BMEND_FUNCEX(status);
    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}