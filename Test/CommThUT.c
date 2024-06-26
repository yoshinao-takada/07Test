#include "BMComm.h"
#include "BMBase.h"
#include <stdio.h>
#include <stdlib.h>
#define PORT_NAME0  "/dev/ttyUSB0"
#define PORT_NAME1  "/dev/ttyUSB1"
#define TEST_BUFSIZE    16

BMStatus_t CommTxThUT();
BMStatus_t CommRxThUT();
BMStatus_t CommFSMUT();

static const BMCommConf_t COMM_CONF0 = { PORT_NAME0, B115200 };
static const BMCommConf_t COMM_CONF1 = { PORT_NAME1, B115200 };
// buffer to reserve variables used duaring testing.
static pthread_t rxth, txth;
static BMComm_t rxcomm, txcomm;
static BMCommThCtx_t txctx;
static BMCommRxThCtx_t rxthctx;
static pthread_mutex_t rbblock = PTHREAD_MUTEX_INITIALIZER;
static BMEvQ_pt txevq;
BMEvQ_SDECL(evq, 4);
BMEv_t ev[] = {
    { BMEVID_RBEMPTY, 0, NULL },
    { BMEVID_RBEMPTY, 0, NULL },
    { BMEVID_RBEMPTY, 0, NULL },
    { BMEVID_RBEMPTY, 0, NULL },
};
static int TestRx_QuitRequest;
static BMEvQ_pt rxevq;
static const uint8_t* MESSAGES[] = {
    "0)0123456789",
    "1)ABCDEFGHIJ",
    "2)abcdefghij",
    "3)+-*/()!#$%",
    "4)0123456789",
    "5)ABCDEFGHIJ",
    "6)abcdefghij",
    "7)+-*/()!#$%",
    "8)0123456789",
    "9)ABCDEFGHIJ",
    "A)abcdefghij",
    "B)+-*/()!#$%",
    "C)0123456789",
    "D)ABCDEFGHIJ",
    "E)abcdefghij",
    "F)+-*/()!#$%",
    "G)0123456789",
    "H)ABCDEFGHIJ",
    "I)abcdefghij",
};
static int wrproh;

/*!
\brief Threading functions in BMComm.h are tested here.
*/
int CommThUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMBufferPool_SInit();
    BMRingBufferPool_SInit();
    BMEvQPool_SInit();
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
    BMBufferPool_SDeinit();
    BMRingBufferPool_SDeinit();
    BMEvQPool_SDeinit();
    BMEND_FUNCEX(status);
    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}


#pragma region CommTxThUT_and_its_helper_functions
// minimal read thread func for testing CommTxTh thread.
void* CommTxThUT_TestRx(void* param)
{
    BMComm_cpt comm = (BMComm_cpt)param;
    uint8_t buf[TEST_BUFSIZE], render_buf[TEST_BUFSIZE + 1];
    int _errno = 0;
    TestRx_QuitRequest = 0;
    for (; TestRx_QuitRequest == 0;)
    {
        BMLOCKED_PRINTF("calling read() @ %s,%d\n", __FUNCTION__, __LINE__);
        ssize_t readlen = read(comm->fd, buf, TEST_BUFSIZE);
        if (readlen == 0)
        { // read() timeout.
            BMLOCKED_PRINTF("readlen == 0 & errno = %d\n", errno);
            continue;;
        }
        else if (readlen < 0)
        {
            BMERR_LOGBREAKEX("Fail in read(), errno = %d\n", errno);
        }
        int i = 0;
        for (; i < readlen; i++)
        {
            render_buf[i] = (buf[i] < ' ') ? '.' : buf[i];
        }
        render_buf[i] = '\0';
        BMLOCKED_PRINTF("buf = %s", render_buf);
        for (i = 0; i < readlen; i++)
        {
            printf("%c%02x", (i == 0) ? ':' : ' ', buf[i]);
        }
        BMLOCKED_PRINTF("\n");
    }
    BMLOCKED_PRINTF("%s exits.\n", __FUNCTION__);
    return param;
}

// Step 2: start test read thread
static BMStatus_t CommTxThUT_StartRead()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        status = BMComm_Open(&COMM_CONF0, &rxcomm);
        if (status)
        {
            BMERR_LOGBREAKEX("Fail in BMComm_Open()");
        }
        if (pthread_create(&rxth, NULL, CommTxThUT_TestRx, (void*)&rxcomm))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in pthread_create()");
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

// Step 3: start write thread under test.
static BMStatus_t CommTxThUT_StartWrite()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        if (pthread_mutex_lock(&rbblock))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in pthread_mutex_unlock(&rbblock)");
        }
        if (BMSTATUS_SUCCESS != (status = BMComm_Open(&COMM_CONF1, &txcomm)))
        {
            BMERR_LOGBREAKEX("Fail in BMComm_Open()");
        }
        if (BMSTATUS_SUCCESS !=
            (status = BMCommThCtx_Init(&txctx, &txcomm, &rbblock,
                txevq = BMEvQPool_SGet())))
        {
            BMERR_LOGBREAKEX("Fail in BMCommThCtx_Init()");
        }
        if (pthread_create(&txth, NULL, BMComm_TxTh, &txctx))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in pthread_create()");
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}
// Step 4: send test byte sequences.
static BMStatus_t CommTxThUT_SendBytes()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    int messageIndex = 0;
    do {
        // send the 1st message.
        int bytesToSend = strlen(MESSAGES[messageIndex]);
        BMRingBuffer_Puts(txctx.rb, MESSAGES[messageIndex++], bytesToSend);
        pthread_mutex_unlock(&rbblock);
        BMEv_pt txev;
        while (1 != BMEvQ_Get(txevq, &txev)) ;
        printf("txev was obtained.\n");
        txev->listeners--;
        printf("txev->listeners = %d\n", txev->listeners);

        // send the 2nd message.
        bytesToSend = strlen(MESSAGES[messageIndex]);
        BMRingBuffer_Puts(txctx.rb, MESSAGES[messageIndex++], bytesToSend);
        pthread_mutex_unlock(&rbblock);
        while (1 != BMEvQ_Get(txevq, &txev)) ;
        printf("txev was obtained.\n");
        txev->listeners--;
        printf("txev->listeners = %d\n", txev->listeners);

        // send the 3rd message.
        bytesToSend = strlen(MESSAGES[messageIndex]);
        BMRingBuffer_Puts(txctx.rb, MESSAGES[messageIndex++], bytesToSend);
        pthread_mutex_unlock(&rbblock);
        while (1 != BMEvQ_Get(txevq, &txev)) ;
        printf("txev was obtained.\n");
        txev->listeners--;
        printf("txev->listeners = %d\n", txev->listeners);
        sleep(2);
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

// Step 5: stop write thread
static BMStatus_t CommTxThUT_StopWrite()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    void* thread_return = NULL;
    do {
        // reset continue request of tx thread.
        txctx.cont = 0;
        // unblock tx thread loop
        if (pthread_mutex_unlock(&rbblock))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in pthread_mutex_unlock(&rbblock)");
        }
        if (pthread_join(txth, &thread_return))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in pthread_join(txth, &thread_return)");
        }
        txth = 0;

        // close Tx serialport.
        if (close(txcomm.fd))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in close(txcomm.fd)");
        }
        txcomm.fd  = -1;

        // return rb, buffer, and evq to their pools.
        if (BMSTATUS_SUCCESS != 
            (status = BMBufferPool_SReturn(txctx.buffer)))
        {
            BMERR_LOGBREAKEX(
                "Fail in BMBufferPool_SReturn(txctx.buffer)");
        }
        txctx.buffer = NULL;
        if (BMSTATUS_SUCCESS != 
            (status = BMRingBufferPool_SReturn(txctx.rb)))
        {
            BMERR_LOGBREAKEX("Fail in BMBufferPool_SReturn(txctx.rb)");
        }
        txctx.rb = NULL;
        if (BMSTATUS_SUCCESS !=
            (status = BMEvQPool_SReturn(txctx.evq)))
        {
            BMERR_LOGBREAKEX("Fail in BMEvQPool_SReturn(txctx.rb)");
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

// Step 6: stop read thread
static BMStatus_t CommTxThUT_StopRead()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    void* thread_return = NULL;
    do {
        BMLOCKED_PRINTF
        ("Setting TestRx_QuitRequest @ %s,%d\n", __FUNCTION__, __LINE__);
        TestRx_QuitRequest = 1;
        // if (pthread_kill(rxth, SIGUSR1))
        // {
        //     status = BMSTATUS_INVALID;
        //     BMERR_LOGBREAKEX("Fail in pthread_kill(rxth, SIGUSR1)");
        // }
        BMLOCKED_PRINTF
        ("calling pthread_join(rxth) @ %s,%d\n", __FUNCTION__, __LINE__);
        if (pthread_join(rxth, &thread_return))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in pthread_join(rxth, )");
        }
        if (close(rxcomm.fd))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in close(rxcomm.fd)");
        }
        rxcomm.fd = -1;
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

/*!
\brief Test CommTxTh thread function
Step 1: set SIGUSR1 handler to doing nothing one.
Step 2: start test read thread
Step 3: start write thread under test.
Step 4: send test byte sequences.
Step 5: stop write thread
Step 6: stop read thread
Step 7: reset SIGUSR1 handler.
*/
BMStatus_t CommTxThUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        // Step 2: start test read thread
        if (status = CommTxThUT_StartRead())
        {
            BMERR_LOGBREAKEX("Fail in CommTxThUT_StartRead()");
        }

        // Step 3: start write thread under test.
        if (status = CommTxThUT_StartWrite())
        {
            BMERR_LOGBREAKEX("Fail in CommTxThUT_StartWrite()");
        }

        // Step 4: send test byte sequences.
        if (status = CommTxThUT_SendBytes())
        {
            BMERR_LOGBREAKEX("Fail in CommTxThUT_SendBytes()");
        }
        // Step 5: stop write thread
        if (status = CommTxThUT_StopWrite())
        {
            BMERR_LOGBREAKEX("Fail in CommTxThUT_StopWrite()");
        }

        // Step 6: stop read thread
        if (status = CommTxThUT_StopRead())
        {
            BMERR_LOGBREAKEX("Fail in CommTxThUT_StopRead()");
        }
   } while (0);
    BMEND_FUNCEX(status);
    return status;
}
#pragma endregion CommTxThUT_and_its_helper_functions

#pragma region CommRxThUT_and_its_helper_functions
// Step 1: Open comm port and init Rx context
BMStatus_t CommRxTh_InitRxCtx()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        if (BMSTATUS_SUCCESS != (status = BMComm_Open(&COMM_CONF0, &rxcomm)))
        {
            BMERR_LOGBREAKEX("Fail in BMComm_Open()");
        }
        status = BMCommRxThCtx_Init(&rxthctx, &rxcomm, &wrproh, &rbblock,
            BMEvQPool_SGet(), BMDispatchers_SGet(0));
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

// Step 2: Start Rx thread.
BMStatus_t CommRxTh_StartRx()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        if (pthread_create(&rxth, NULL, BMComm_RxTh, &rxthctx))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in pthread_create()");
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}
// Step 3: Open another commport for main thread as Tx thread.
BMStatus_t CommRxTh_OpenTx()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        if (BMSTATUS_SUCCESS != (status = BMComm_Open(&COMM_CONF1, &txcomm)))
        {
            BMERR_LOGBREAKEX("Fail in BMComm_Open()");
        }
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

BMStatus_t CommRxTh_Write(BMComm_cpt comm, const uint8_t* msg)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    ssize_t writtenLen = 0;
    int remaining = strlen(msg);
    int accWrittenLen = 0;
    for (; status == BMSTATUS_SUCCESS && remaining > 0; )
    {
        writtenLen = write(comm->fd, msg + accWrittenLen, remaining);
        if (writtenLen > 0)
        {
            remaining -= writtenLen;
            accWrittenLen += writtenLen;
        }
    }
    return status;
}

void CommRxTh_ShowRxMessage(BMRingBuffer_pt rb, BMBuffer_pt buffer)
{
    uint16_t rbgotten = 0;
    do {
        buffer->filled = buffer->crunched = 0;
        rbgotten = BMRingBuffer_Gets(rb, buffer->buf, buffer->size - 1);
        if (rbgotten)
        {
            buffer->buf[rbgotten] = '\0';
            BMLOCK_STDOUT;
            printf("buf = %s", buffer->buf);
            for (uint16_t i = 0; i < rbgotten; i++)
            {
                printf("%c%02x", i == 0 ? ':' : ' ', buffer->buf[i]);
            }
            printf("\n");
            BMUNLOCK_STDOUT;
        }
    } while (rbgotten);
}

// Step 4: Repeat Tx and Rx several times.
BMStatus_t CommRxTh_SendMsg()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMEv_pt ev;
    BMBuffer_pt buffer = BMBufferPool_SGet(BMBufferPoolType_SHORT);
    int msgidx = 0, msgidxmax = ARRAYSIZE(MESSAGES) * 2;
    do {
        BMDispatchers_SCrunchEvent();
        if (0 == (msgidx & 1))
        {
            const uint8_t* msg = MESSAGES[msgidx >> 1];
            BMLOCKED_PRINTF("msg = %s\n", msg);
            status = CommRxTh_Write(&txcomm, msg);
            msgidx++;
        }

        // wait for Rx event queue being ready.
        if ((msgidx & 1) && (1 == BMEvQ_Get(rxthctx.base.evq, &ev)))
        {
            CommRxTh_ShowRxMessage(rxthctx.base.rb, buffer);
            ev->listeners--;
        }

        if ((msgidx & 1) && (*(rxthctx.wrproh) == 0))
        {
            BMLOCKED_PRINTF("msgidx = %d\n", msgidx);
            msgidx++;
        }
        pause();
    } while (msgidx < msgidxmax);
    BMEND_FUNCEX(status);
    return status;
}

// Step 5: Close Tx comm port.
BMStatus_t CommRxTh_CloseTx()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        BMComm_Close(&txcomm);
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

// Step 6: Stop Rx thread and close comm port.
BMStatus_t CommRxTh_StopCloseRx()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    void* threadReturn = NULL;
    do {
        // reset continuous flag
        rxthctx.base.cont = 0;
        // BMLOCKED_PRINTF("rxthctx.base.cont = 0 done\n");

        if (pthread_join(rxth, &threadReturn))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in pthread_join(rxth,)");
        }

        BMComm_Close(&rxcomm);
    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

// Step 1: Open comm port and init Rx context
// Step 2: Start Rx thread.
// Step 3: Open another commport for main thread as Tx thread.
// Step 4: Repeat Tx and Rx several times.
// Step 5: Close Tx comm port.
// Step 6: Stop Rx thread and close comm port.
BMStatus_t CommRxThUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    BMDispatchers_SInit(BMTICK_MIN_PERIOD);
    BMEvQPool_SInit();
    BMBufferPool_SInit();
    BMRingBufferPool_SInit();
    do {
        if (BMSTATUS_SUCCESS != (status = CommRxTh_InitRxCtx()))
        {
            BMERR_LOGBREAKEX("Fail in CommRxTh_InitRxCtx()");
        }
        if (BMSTATUS_SUCCESS != (status = CommRxTh_StartRx()))
        {
            BMERR_LOGBREAKEX("Fail in CommRxTh_StartRx()");
        }
        if (BMSTATUS_SUCCESS != (status = CommRxTh_OpenTx()))
        {
            BMERR_LOGBREAKEX("Fail in CommRxTh_OpenTx()");
        }
        if (BMSTATUS_SUCCESS != (status = CommRxTh_SendMsg()))
        {
            BMERR_LOGBREAKEX("Fail in CommRxTh_SendMsg()");
        }
        if (BMSTATUS_SUCCESS != (status = CommRxTh_CloseTx()))
        {
            BMERR_LOGBREAKEX("Fail in CommRxTh_CloseTx()");
        }
        if (BMSTATUS_SUCCESS != (status = CommRxTh_StopCloseRx()))
        {
            BMERR_LOGBREAKEX("Fail in CommRxTh_StopCloseRx()");
        }
    } while (0);
    BMDispatchers_SDeinit();
    BMEvQPool_SDeinit();
    BMBufferPool_SDeinit();
    BMRingBufferPool_SDeinit();
    BMEND_FUNCEX(status);
    return status;
}
#pragma endregion CommRxThUT_and_its_helper_functions

BMStatus_t CommFSMUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {

    } while (0);
    BMEND_FUNCEX(status);
    return status;
}

