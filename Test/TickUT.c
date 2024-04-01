#include "../Lib/BMTick.h"
#define DISPATCHER_COUNT    4
BMDispatchers_DECL(disp, DISPATCHER_COUNT);

// check macro definition of BMDispatchers_DECL
static BMStatus_t CheckDisp()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        if (disp.count != DISPATCHER_COUNT)
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "disp.count != DISPATCHER_COUNT");
        }
        if (disp.dispatchers != disp_dispatchers)
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "disp.dispatchers != disp_dispatchers");
        }
        if (disp.q != &disp_Q)
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "disp.q != &disp_Q");
        }
        if (disp.q->qbase.count != BMDISPATCHERS_QUEUESIZE)
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "disp.q->qbase.count != BMDISPATCHERS_QUEUESIZE");
        }
        if ((disp.q->qbase.rdidx != 0) ||
            (disp.q->qbase.wridx != 0)) 
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "disp.q->qbase.rdidx or .wridx != 0");
        }
    } while (0);
    return status;
}

#pragma region Timer_Dispatcher_handlers
static int param0 = 0;
static int param1 = 0;
static int param2 = 0;
static int param3 = 0;

static void* TimerHandler0(void* param)
{
    int* iparam = (int*)param;
    printf("param0 = %d\n", *iparam);
    (*iparam)++;
    return param;
}

static void* TimerHandler1(void* param)
{
    int* iparam = (int*)param;
    printf("param1 = %d\n", *iparam);
    (*iparam)++;
    return param;
}

static void* TimerHandler2(void* param)
{
    int* iparam = (int*)param;
    printf("param2 = %d\n", *iparam);
    (*iparam)++;
    return param;
}

static void* TimerHandler3(void* param)
{
    int* iparam = (int*)param;
    printf("param3 = %d\n", *iparam);
    (*iparam)++;
    return param;
}

typedef void* (*TimerHandler_f)(void*);

static TimerHandler_f handlers[] =
{
    TimerHandler0, TimerHandler1, TimerHandler2, TimerHandler3
};

static void* params[] =
{
    (void*)&param0, (void*)&param1, (void*)&param2, (void*)&param3
};

// decimation ratios to base interval
static uint32_t intervals[] = { 200, 250, 300, 350 };

static void SetupDispatchers(BMDispatchers_pt dispatchers)
{
    for (int i = 0; i < ARRAYSIZE(handlers); i++)
    {
        BMDispatcher_Set(dispatchers->dispatchers + i,
            intervals[i], intervals[i], handlers[i], params[i]);
        printf("BMDispatcher_Set(%d) finished.\n", i);
    }
}
#pragma endregion Timer_Dispatcher_handlers

int TickUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMDispatchers_INIT(disp);
    do {
        if (BMSTATUS_SUCCESS != (status = CheckDisp())) break;
        SetupDispatchers(&disp);
        // set base interval to 1ms.
        if (BMSTATUS_SUCCESS != (status = BMSystick_Init(disp.q, 1))) break;
        for (;param0 < 100;)
        {
            status = BMDispatchers_CrunchEvent(&disp);
            if (BMSTATUS_SUCCESS != status)
            {
                BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                    "Fail in BMDispatchers_CrunchEvent(&disp)");
            }
            pause(); // wait for the process to be signaled.
        }
    } while (0);
    BMEND_FUNC(__FILE__, __FUNCTION__, __LINE__,status);
    BMSystick_Deinit();
    BMDispatchers_DEINIT(disp);
    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}