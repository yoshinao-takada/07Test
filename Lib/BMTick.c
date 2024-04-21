#include "BMTick.h"
#include <signal.h>
#include <sys/param.h>

#pragma region TIME_STRUCT_CONVERSION
/*!
\brief convert Milliseconds to timeval
*/
static
void BMTimeval_FromMillisec(struct timeval* tv, uint32_t Millisec)
{
    tv->tv_sec = Millisec / 1000;
    tv->tv_usec = 1000 * (Millisec - 1000 * tv->tv_sec);
}

/*!
\brief convert seconds to timeval
*/
static
void BMTimeval_FromSec(struct timeval* tv, uint32_t sec)
{
    tv->tv_sec = sec;
    tv->tv_usec = 0;
}

/*!
\brief convert Milliseconds to itimerval
*/
static
void BMITimerval_FromMillisec(struct itimerval* tv, uint32_t Millisec)
{
    BMTimeval_FromMillisec(&tv->it_interval, Millisec);
    BMTimeval_FromMillisec(&tv->it_value, Millisec);
}

/*!
\brief convert seconds to itimerval
*/
static
void BMITimerval_FromSec(struct itimerval* tv, uint32_t sec)
{
    BMTimeval_FromSec(&tv->it_interval, sec);
    BMTimeval_FromSec(&tv->it_value, sec);
}
#pragma endregion TIME_STRUCT_CONVERSION

#pragma region DISPATCHER_FUNCTIONS
void BMDispatcher_Set(BMDispatcher_pt dispatcher,
    uint32_t count, uint32_t init,
    void* (*_handler)(void*), void* param)
{
    dispatcher->handler = _handler;
    dispatcher->param = param;
    dispatcher->init = init;
    dispatcher->count = count;
}

void BMDispatcher_Reset(BMDispatcher_pt dispatcher)
{
    dispatcher->count = dispatcher->init = 0;
    dispatcher->handler = NULL;
    dispatcher->param = NULL;
}

BMStatus_t BMDispatcher_Dispatch(BMDispatcher_pt dispatcher)
{
    if (dispatcher->count && --dispatcher->count == 0)
    {
        dispatcher->count = dispatcher->init;
        dispatcher->result = dispatcher->handler(dispatcher->param);
        return (dispatcher->result == dispatcher->param) ?
            BMSTATUS_SUCCESS : BMSTATUS_INVALID;
    }
    return BMSTATUS_SUCCESS;
}
#pragma endregion DISPATCHER_FUNCTIONS

void BMDispatchers_Clear(BMDispatchers_pt dispatchers)
{
    for (uint16_t i = 0; i < dispatchers->count; i++)
    {
        BMDispatcher_Reset(dispatchers->dispatchers + i);
    }
}

BMStatus_t BMDispatchers_Dispatch(BMDispatchers_pt dispatchers)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    for (uint16_t i = 0; i < dispatchers->count; i++)
    {
        BMStatus_t individual_status =
            BMDispatcher_Dispatch(dispatchers->dispatchers + i);
        if (!status && individual_status)
        {
            status = individual_status;
        }
    }
    return status;
}

BMStatus_t BMDispatchers_CrunchEvent(BMDispatchers_pt dispatchers)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMEv_pt ev = NULL;
    do {
        if (0 == BMEvQ_Get(dispatchers->q, &ev)) break; // no event
        status = BMDispatchers_Dispatch(dispatchers);
        ev->listeners--;
    } while (0);
    return status;
}

BMDispatchers_SDECL(sdisps, BMDISPATCHERS_STATIC_SIZE);

void BMDispatchers_SInit(uint16_t tickPeriod)
{
    BMDispatchers_INIT(sdisps);
    BMSystick_Init(sdisps.q, BMTICK_MIN_PERIOD);
}

void BMDispatchers_SDeinit()
{
    BMDispatchers_DEINIT(sdisps);
}

BMStatus_t BMDispatchers_SCrunchEvent()
{
    return BMDispatchers_CrunchEvent(&sdisps);
}

BMDispatcher_pt BMDispatchers_Get(uint16_t index)
{
    return (index < sdisps.count) ? sdisps.dispatchers + index : NULL;
}

static BMEv_t ev = { BMEVID_TICK, 0, NULL };

static BMEvQ_pt dispatcher_queue;

/*!
\brief if ev is available and dispatcher_queue was initialized,
    put ev into the queue;
    i.e. triggering the dispatcher bound to the queue.
*/
static void handler(int sig)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        if (ev.listeners)
        {
            status = BMSTATUS_TIMEOUT;
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "event object busy");
        }
        assert(dispatcher_queue);
        uint16_t count = BMEvQ_Put(dispatcher_queue, &ev);
        if (count == 0)
        {
            status = BMSTATUS_BUFFUL;
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "event queue full");
        }        
    } while (0);
    if (status)
    {
        exit(1);
    }
}

static struct itimerval itold;
static struct itimerval itnew;

BMStatus_t BMSystick_Init(BMEvQ_pt evq, uint16_t period)
{
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = handler;
    BMStatus_t status = BMSTATUS_SUCCESS;
    dispatcher_queue = evq;
    do {
        // set SIGALRM handler
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGALRM, &sa, NULL))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in sigaction()");
        }

        // start timer
        BMITimerval_FromMillisec(&itnew, MIN(period, BMTICK_MIN_PERIOD));
        if (setitimer(ITIMER_REAL, &itnew, &itold))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in setitimer()");
        }
    } while (0);
    return status;
}

BMStatus_t BMSystick_Deinit()
{
    struct itimerval itnew;
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        if (setitimer(ITIMER_REAL, &itold, &itnew))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in setitimer()");
        }
    } while (0);
    return status;
}

const struct timeval* BMSystick_GetInterval()
{
    return &itnew.it_interval;
}

double BMSystick_GetIntervalDouble()
{
    return 1.0e-6 * BMSystick_GetInterval()->tv_usec;
}