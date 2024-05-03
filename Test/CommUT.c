#include "BMComm.h"
#include <signal.h>
#include <sys/param.h>

#define COMMUT_BUFSIZE 32
static BMComm_t
    comm0 = { -1, 0.0 },
    comm1 = { -1, 0.0 };
static BMCommConf_t
    conf0 = { "/dev/ttyUSB0", B115200 },
    conf1 = { "/dev/ttyUSB1", B115200 };

void SigIntHandler(int)
{
    fprintf(stderr, "%s called.\n", __FUNCTION__);
}

void* ReadThread(void* param)
{
    BMComm_pt comm = (BMComm_pt)param;
    char buf[COMMUT_BUFSIZE];
    do {
        memset(buf, 0, COMMUT_BUFSIZE);
        fprintf(stderr, "reading comm @ %s,%s,%d\n",
            __FILE__, __FUNCTION__, __LINE__);
        ssize_t readlen = read(comm->fd, buf, COMMUT_BUFSIZE);
        if (readlen > 0)
        {
            fprintf(stderr, "has read \"%s\" comm @ %s,%s,%d\n", buf,
                __FILE__, __FUNCTION__, __LINE__);
        }
        else
        {
            fprintf(stderr, "readlen = %d @ %s,%s,%d\n",
                readlen, __FILE__, __FUNCTION__, __LINE__);
        }
    } while (1);
    return param;
}

BMStatus_t RegSigAction()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        struct sigaction sa;
        sa.sa_flags = 0;
        sa.sa_handler = SigIntHandler;
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGINT, &sa, NULL))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in sigaction()");
        }
    } while (0);
    return status;
}
int CommUT()
{
    static const uint8_t MESSAGE[] = "message0";
    BMStatus_t status = BMSTATUS_SUCCESS, status2 = BMSTATUS_SUCCESS;
    pthread_t th;
    do {
        if (status = RegSigAction())
        {
            BMERR_LOGBREAKEX("RegSigAction() ");
        }
        status = BMComm_Open(&conf0, &comm0);
        status2 = BMComm_Open(&conf1, &comm1);
        if (status || status2)
        {
            status = MAX(status, status2);
            BMERR_LOGBREAKEX("Fail in BMComm_Open()");
        }
        if (pthread_create(&th, NULL, ReadThread, &comm0))
        {
            status = BMSTATUS_INVALID;
            BMERR_LOGBREAKEX("Fail in pthread_create() ");
        }
        // write Tx message
        sleep(1);
        write(comm1.fd, MESSAGE, sizeof(MESSAGE));
        // break blocked thread by read().
        sleep(1);
        pthread_kill(th, SIGINT);
        // write Tx message again
        sleep(1);
        write(comm1.fd, MESSAGE, sizeof(MESSAGE));
    } while (0);
    BMComm_Close(&comm0);
    BMComm_Close(&comm1);
    BMEND_FUNCEX(status);
    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}