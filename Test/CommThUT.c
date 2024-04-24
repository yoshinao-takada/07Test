#include "BMComm.h"
#include "BMBase.h"
#include <stdio.h>
#include <stdlib.h>

BMStatus_t CommTxThUT();
BMStatus_t CommRxThUT();
BMStatus_t CommFSMUT();

/*!
\brief Threading functions in BMComm.h are tested here.
*/
int CommThUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        if ((status = CommTxThUT()) != BMSTATUS_SUCCESS)
        {
            BMERR_LOGBREAKEX("Fail in CommTxThUT()");
        }
        if ((status = CommRxThUT()) != BMSTATUS_SUCCESS)
        {
            BMERR_LOGBREAKEX("Fail in CommRxThUT()");
        }
        if ((status = CommFSMUT()) != BMSTATUS_SUCCESS)
        {
            BMERR_LOGBREAKEX("Fail in CommFSMUT()");
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}


BMStatus_t CommTxThUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {

    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

BMStatus_t CommRxThUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {

    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

BMStatus_t CommFSMUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {

    } while (0);
    BMEND_FUNCEX(status);
    return status;
}