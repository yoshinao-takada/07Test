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
        if (pthread_spin_init(&ctx->wrproh, PTHREAD_PROCESS_PRIVATE))
        {
            status = BMSTATUS_INVALID;
            break;
        }
        ctx->txctx.wrproh = ctx->rxctx.base.wrproh = &ctx->wrproh;

        // init buffers
        ctx->rxctx.base.buffer = BMBufferPool_SGet(BMBufferPoolType_SHORT);
        ctx->txctx.buffer = BMBufferPool_SGet(BMBufferPoolType_SHORT);
        ctx->rxctx.base.rb = BMRingBufferPool_SGet(BMRingBufferPoolType_SHORT);
        ctx->txctx.rb = BMRingBufferPool_SGet(BMRingBufferPoolType_SHORT);

        // init one-shot timer

    } while (0);
    return status;
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
    while (ctx_->base.cont)
    {
        // prohibit tx and start 1-shot delay
        pthread_spin_lock(ctxbase->wrproh);
        ctx_->oneshot->count = wrprohIni;
        readResult = read(ctxbase->base.fd, ctxbase->buffer->buf,
            ctxbase->buffer->size);
        if (readResult < 0) continue; // sth invalid occured.
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
        }
    }
    return ctx;
}
BMStatus_t BMCommThCtx_Init(BMCommThCtx_pt ctx, BMComm_cpt comm,
    pthread_spinlock_t* wrproh, BMEvQ_pt evq)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        memcpy(&ctx->base, comm, sizeof(BMComm_t));
        ctx->wrproh = wrproh;
        ctx->evq = evq;
        ctx->buffer = BMBufferPool_SGet(BMBufferPoolType_SHORT);
        ctx->rb = BMRingBufferPool_SGet(BMRingBufferPoolType_SHORT);
        ctx->ev.listeners = 0;
        ctx->ev.param = ctx->rb;
        ctx->ev.id = BMEVID_RBEMPTY;
    } while (0);
    return status;
}

BMStatus_t BMCommRxThCtx_Init(BMCommRxThCtx_pt ctx, BMComm_cpt comm,
    pthread_spinlock_t* wrproh, BMEvQ_pt evq, BMDispatcher_pt oneshot)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        status = BMCommThCtx_Init(&ctx->base, comm, wrproh, evq);
        if (BMSTATUS_SUCCESS != status)
        {
            break;
        }
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
            pause();
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

