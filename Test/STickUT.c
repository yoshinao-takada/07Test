#include "BMTick.h"
static int _i[BMDISPATCHERS_STATIC_SIZE] = { 0, 0, 0, 0 };

void* hdlr0(void* arg)
{
    int* _arg = (int*)arg;
    printf("*_arg = %d @ %s\n", *_arg, __FUNCTION__);
    *_arg = *_arg + 1;
    return arg;
}

void* hdlr1(void* arg)
{
    int* _arg = (int*)arg;
    printf("*_arg = %d @ %s\n", *_arg, __FUNCTION__);
    *_arg = *_arg + 1;
    return arg;
}

void* hdlr2(void* arg)
{
    int* _arg = (int*)arg;
    printf("*_arg = %d @ %s\n", *_arg, __FUNCTION__);
    *_arg = *_arg + 1;
    return arg;
}

void* hdlr3(void* arg)
{
    int* _arg = (int*)arg;
    printf("*_arg = %d @ %s\n", *_arg, __FUNCTION__);
    *_arg = *_arg + 1;
    return arg;
}

static void* (*hdlrs[])(void*) = { hdlr0, hdlr1, hdlr2, hdlr3 };
static const uint32_t INTERVALS[] = { 100, 150, 200, 250 };
int STickUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        for (uint16_t i = 0; i < BMDISPATCHERS_STATIC_SIZE; i++)
        {
            BMDispatcher_pt disp = BMDispatchers_SGet(i);
            disp->count = disp->init = INTERVALS[i];
            disp->param = _i + i;
            disp->handler = hdlrs[i];
        }
        sleep(5);
        BMDispatchers_SInit(BMTICK_MIN_PERIOD);
        do {
            BMStatus_t status = BMDispatchers_SCrunchEvent();
            if (status)
            {
                BMERR_LOGBREAKEX("Fail in BMDispatchers_SCrunchEvent()\n");
            }
            pause();
        } while (_i[0] < 100);
    } while (0);
    BMEND_FUNCEX(status);
    BMDispatchers_SDeinit();
    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}