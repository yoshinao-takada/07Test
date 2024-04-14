#include "BMSched.h"
BMStatus_t BMSched_Init(BMSched_pt sched)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        for (uint16_t i = 0; i < sched->size; i++)
        {
            sched->fsms[i] = NULL;
        }
    } while (0);
    return status;
}

int16_t BMSched_1stAvailable(BMSched_pt sched)
{
    int16_t result = -1;
    for (uint16_t idx = 0; idx < sched->size; idx++)
    {
        if (sched->fsms[idx] == NULL)
        {
            result = (int16_t)idx;
            break;
        }
    }
    return result;
}

static void MainSigIntHandler(int sig)
{
    // does nothing. Only breaks pause().
}


void BMSched_Main(BMSched_pt sched)
{
    for (;;)
    {

        pause();
    }
}