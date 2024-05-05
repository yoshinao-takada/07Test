#include "BMComm.h"
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
        // program wait for deciseconds of c_cc[VTIME]
        // in the following case, timeout is 10 deciseconds = 1 sec.
        opt.c_cc[VMIN] = 0;
        opt.c_cc[VTIME] = 10;
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

BMStatus_t BMCommCtx_Init
(BMCommCtx_pt ctx, BMComm_cpt comm, BMDispatchers_pt dispather,
 BMEvQ_pt phyQ)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        // init serialport file descriptors
        memcpy(&ctx->rxctx.base.base, comm, sizeof(BMComm_t));
        memcpy(&ctx->txctx.base, comm, sizeof(BMComm_t));

        // init write prohibit
        {
            pthread_mutex_t mtxtemp = PTHREAD_MUTEX_INITIALIZER;
            memcpy(&ctx->wrproh, &mtxtemp, sizeof(pthread_mutex_t));
            memcpy(&ctx->rbblock, &mtxtemp, sizeof(pthread_mutex_t));
        }
        ctx->txctx.rbblock = ctx->rxctx.base.rbblock = &ctx->rbblock;

        // init buffers
        ctx->rxctx.base.buffer = BMBufferPool_SGet(BMBufferPoolType_SHORT);
        ctx->txctx.buffer = BMBufferPool_SGet(BMBufferPoolType_SHORT);
        ctx->rxctx.base.rb = BMRingBufferPool_SGet(BMRingBufferPoolType_SHORT);
        ctx->txctx.rb = BMRingBufferPool_SGet(BMRingBufferPoolType_SHORT);

        // init one-shot timer

    } while (0);
    return status;
}

// BMDispatcher, timer dispatcher handler to reset
// wrproh flag.
static void* ResetWrproh(void* wrproh)
{
    int* wrproh_ = (int*)wrproh;
    *wrproh_ = 0;
    BMLOCKED_PRINTF("%s called.\n", __FUNCTION__);
    return wrproh;
}

void* BMComm_RxTh(void* ctx)
{
    BMCommRxThCtx_pt ctx_ = (BMCommRxThCtx_pt)ctx;
    BMCommThCtx_pt ctxbase = (BMCommThCtx_pt)&ctx_->base;
    double tickInterval = BMSystick_GetIntervalDouble();
    int waitByteCount = 2 * BMBUFFERPOOL_SHORTBUFFERSIZE;
    double waitTime = waitByteCount * ctxbase->base.secPerByte;
    int wrprohIni = MAX((int)ceil(waitTime / tickInterval), 1);
    ssize_t readResult = -1;
    ctx_->oneshot->param = ctx_->wrproh;
    ctx_->oneshot->handler = ResetWrproh;
    ctx_->base.cont = 1;
    *(ctx_->wrproh) = 1; // prohibit to transit to WR state.
    while (ctx_->base.cont)
    {
        // prohibit tx and start 1-shot delay
        ctx_->oneshot->count = wrprohIni;
        BMLOCKED_PRINTF("ctx_->wrproh = %d\n", *(ctx_->wrproh));
        readResult = read(ctxbase->base.fd, ctxbase->buffer->buf,
            ctxbase->buffer->size);
        if (readResult == 0) continue; // timeout
        else if (readResult < 0) 
        { // error
            BMERR_LOGBREAKEX("Fail in read()");
        }
        else if (readResult > 0)
        { // read success
            uint16_t rbappended = 
                BMRingBuffer_Puts(ctxbase->rb, ctxbase->buffer->buf, 
                    (uint16_t)readResult);
            if (rbappended != (uint16_t)readResult)
            {
                printf("RingBuffer overflow @ %s,%s,%d\n",
                    __FILE__, __FUNCTION__, __LINE__);
                // clear ringbuffer
                ctxbase->rb->base.rdidx = ctxbase->rb->base.wridx = 0;
            }
            else if (ctxbase->ev.listeners == 0)
            { // send an event to PHY FSM if no error && event is available
                BMEvQ_Put(ctxbase->evq, &ctxbase->ev);
            }
            *(ctx_->wrproh) = 1;
        }
    }
    return ctx;
}

BMStatus_t BMCommThCtx_Init(BMCommThCtx_pt ctx, BMComm_cpt comm,
    pthread_mutex_t* rbblock, BMEvQ_pt evq)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        memcpy(&ctx->base, comm, sizeof(BMComm_t));
        ctx->rbblock = rbblock;
        ctx->evq = evq; // evq is usually supplied by downstream FSM.
        ctx->buffer = BMBufferPool_SGet(BMBufferPoolType_SHORT);
        ctx->rb = BMRingBufferPool_SGet(BMRingBufferPoolType_SHORT);
        ctx->ev.listeners = 0;
        ctx->ev.param = ctx->rb;
        ctx->ev.id = BMEVID_RBEMPTY;
        ctx->cont = 1;
    } while (0);
    return status;
}

BMStatus_t BMCommRxThCtx_Init(BMCommRxThCtx_pt ctx, BMComm_cpt comm,
    int* wrproh, pthread_mutex_t* rbblock, BMEvQ_pt evq,
    BMDispatcher_pt oneshot)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        status = BMCommThCtx_Init(&ctx->base, comm, rbblock, evq);
        if (BMSTATUS_SUCCESS != status)
        {
            break;
        }
        ctx->wrproh = wrproh;
        ctx->oneshot = oneshot;
    } while (0);
    return status;
}
    

void* BMComm_TxTh(void* ctx)
{
    BMCommThCtx_pt ctx_ = (BMCommThCtx_pt)ctx;
    while (ctx_->cont)
    {
        if (BMRingBuffer_ISEMPTY(ctx_->rb))
        {
            // Notify PHY FSM of Tx-RB empty.
            BMEvQ_Put(ctx_->evq, &ctx_->ev);
            // wait on rbblock mutex.
            pthread_mutex_trylock(ctx_->rbblock);
            pthread_mutex_lock(ctx_->rbblock);
        }
        ctx_->buffer->filled = BMRingBuffer_Gets(
            ctx_->rb, ctx_->buffer->buf, ctx_->buffer->size);
        uint16_t remaining = ctx_->buffer->filled;
        while (remaining)
        {
            ssize_t written = write(ctx_->base.fd,
                ctx_->buffer->buf + ctx_->buffer->crunched, remaining);
            if (written > 0)
            {
                remaining -= written;
                ctx_->buffer->crunched += written;
            }
        }
        ctx_->buffer->filled = ctx_->buffer->crunched = 0;
    }
    return ctx;
}

