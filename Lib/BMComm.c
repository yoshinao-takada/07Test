#include "BMComm.h"
#include "BMFSM.h"
#include "BMCRC.h"
#include <sys/param.h>
#include <math.h>

#pragma region BAUDRATE_UTILITY
static const int BAUD_MAP[9][2] =
{
    { 1200, B1200 },
    { 2400, B2400 },
    { 4800, B4800 },
    { 9600, B9600 },
    { 19200, B19200 },
    { 38400, B38400 },
    { 57600, B57600 },
    { 115200, B115200 },
    { 230400, B230400 },
};

int BMBaudDesc_ToBaudrate(int bauddesc)
{
    int result = 0;
    for (int index = 0; index < ARRAYSIZE(BAUD_MAP); index++)
    {
        if (bauddesc == BAUD_MAP[index][1])
        {
            result = BAUD_MAP[index][0];
            break;
        }
    }
    return result;
}

int BMBaudDesc_FromBaudrate(int baudrate)
{
    int result = B0;
    for (int index = 0; index < ARRAYSIZE(BAUD_MAP); index++)
    {
        if (baudrate == BAUD_MAP[index][0])
        {
            result = BAUD_MAP[index][1];
            break;
        }
    }
    return result;
}

BMStatus_t
BMBaudDesc_ToSecPerByte(int fd, double* secPerByte)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    struct termios opt;
    int baudrate;
    do {
        if (tcgetattr(fd, &opt))
        {
            status = BMSTATUS_INVALID;
            break;
        }
        baudrate = BMBaudDesc_ToBaudrate(cfgetispeed(&opt));
        double bittime = 1.0 / baudrate;
        int csize = opt.c_cflag & CSIZE;
        int cbits = 0;
        switch (csize)
        {
            case CS5:
                cbits = 5;
                break;
            case CS6:
                cbits = 6;
                break;
            case CS7:
                cbits = 7;
                break;
            case CS8:
                cbits = 8;
                break;
        }
        cbits += 2; // add start bit and minimum stop bit
        if (0 != (opt.c_cflag & CSTOPB)) cbits++; // additional stop bit
        if (0 != (opt.c_cflag & PARENB)) cbits++; // parity bit
        *secPerByte = bittime * cbits;
    } while (0);
    return status;
}
#pragma endregion BAUDRATE_UTILITY
#pragma region BMComm
BMStatus_t BMComm_Open(BMCommConf_cpt conf, BMComm_pt comm)
{
    struct termios opt;
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        comm->fd = open(conf->devname, O_RDWR | O_NOCTTY /*| O_NONBLOCK*/);
        if (-1 == comm->fd)
        {
            status = BMSTATUS_INVALID;
            break;
        }
        tcgetattr(comm->fd, &opt);
        cfsetispeed(&opt, conf->bauddesc);
        cfsetospeed(&opt, conf->bauddesc);
        opt.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
        opt.c_cflag |= CS8;
        opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        opt.c_iflag &= ~(IXON | IXOFF | IXANY);
        opt.c_iflag &= ~(INLCR | IGNCR | ICRNL | IUCLC);
        opt.c_iflag &= ~(INPCK | ISTRIP);
        opt.c_oflag &= ~OPOST;
        // program wait for infinite time until more than one
        // characters are received.
        opt.c_cc[VMIN] = 1;
        opt.c_cc[VTIME] = 0;
        tcsetattr(comm->fd, TCSANOW, &opt);
        status = BMBaudDesc_ToSecPerByte(comm->fd, &comm->secPerByte);
    } while (0);
    return status;
}

void BMComm_Close(BMComm_pt comm)
{
    if (comm->fd > 0)
    {
        close(comm->fd);
        comm->fd = -1;
        comm->secPerByte = 0.0;
    }
}
#pragma endregion BMComm

#pragma region BMCommRx
static void*
CommRxEnableTx(void* param)
{
    BMCommRx_pt rx = (BMCommRx_pt)param;
    pthread_spin_unlock(&rx->txdisable);
    return param;
}

static void
CommRxSigIntHandler(int sig)
{

}

static void
RegCommRxSigIntHandler()
{
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = CommRxSigIntHandler;
    sigemptyset(&sa.sa_mask) ;
    sigaction(SIGINT, &sa, NULL);
}

void
BMCommRx_Init(BMCommRx_pt Rx, BMCommRxConf_cpt conf, BMComm_cpt comm)
{
    Rx->base.fd = comm->fd;
    Rx->base.secPerByte = comm->secPerByte;
    Rx->rxrb = conf->rxrb;
    Rx->delay_init = MAX(1, 
        (int)floor(0.5 + 1000 * Rx->base.secPerByte / 10));
    Rx->delay = conf->delay;
    Rx->delay->count = Rx->delay->init = 0;
    Rx->delay->handler = CommRxEnableTx;
    Rx->delay->param = Rx;
    Rx->oq = conf->oq;
    Rx->ev.listeners = 0;
    Rx->ev.param = Rx->rxrb;
    Rx->ev.id = 0;
    Rx->quit_request = 0;
    Rx->rxbuflen = RXBUF_LEN;
    pthread_spin_init(&Rx->txdisable, PTHREAD_PROCESS_PRIVATE);

    RegCommRxSigIntHandler();
}

static void*
CommRxThread(void* param) 
{
    BMCommRx_pt rx = (BMCommRx_pt)param;
    while (!rx->quit_request)
    {
        pthread_spin_lock(&rx->txdisable);
        rx->delay->count = rx->delay_init;
        ssize_t readlen = read(rx->base.fd, rx->rxbuf, rx->rxbuflen);
        if (readlen > 0)
        {
            uint16_t putlen = 
                BMRingBuffer_Puts(rx->rxrb, rx->rxbuf, (uint16_t)readlen);
            if (putlen == (uint16_t)readlen)
            {
                rx->delay->count = rx->delay_init; // reset one-shot timer
                if (rx->ev.listeners == 0)
                {
                    assert(1 == BMEvQ_Put(rx->oq, &rx->ev));
                }
            }
            else
            {
                fprintf(stderr, "Ring buffer overflow @ %s,%s,%d\n",
                    __FILE__, __FUNCTION__, __LINE__);
            }
        }
    }
    return param;
}

BMStatus_t BMCommRx_Start(BMCommRx_pt Rx)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        if (pthread_create(&Rx->th, NULL, CommRxThread, (void*)Rx))
        {
            status = BMSTATUS_INVALID;
            break;
        }
    } while (0);
    return status;
}

BMStatus_t BMCommRx_Stop(BMCommRx_pt Rx)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        Rx->quit_request = 1;
        if (pthread_kill(Rx->th, SIGINT))
        {
            status = BMSTATUS_INVALID;
            break;
        }
    } while (0);
    return status;
}
#pragma endregion BMCommRx
#pragma region BMCommTx
BMStateResult_t BMCommTx_Empty(BMFSM_pt fsm, BMEv_pt ev)
{
    BMStateResult_t result = BMStateResult_IGNORE;
    if (ev->id != BMEVID_TXSTART)
    {
        return result;
    }
    BMCommTxCtx_pt ctx = (BMCommTxCtx_pt)(fsm->ctx);
    if (0 == BMBufferQ_Put(ctx->bufq, (BMBuffer_pt)(ev->param)))
    {
        return BMStateResult_ERR;
    }
    else if (pthread_spin_trylock(&ctx->rx->txdisable))
    {
        fsm->state = BMCommTx_Remaining;
        return BMStateResult_TRANSIT;
    }
    BMBuffer_pt buf = BMBufferQ_Peek(ctx->bufq);
    return result;
}

BMStateResult_t BMCommTx_Remaining(BMFSM_pt fsm, BMEv_pt ev)
{
    BMStateResult_t result = BMStateResult_IGNORE;

    return result;
}
#pragma endregion BMCommTx
