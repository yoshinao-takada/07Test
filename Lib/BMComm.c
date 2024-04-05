#include "BMComm.h"
#include <sys/param.h>

#pragma region BAUDRATE_UTILITY
static const int BAUD_MAP[][2] =
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

static BMStatus_t Open(BMCommConf_cpt conf, int* pfd)
{
    struct termios opt;
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        *pfd = open(conf->devname, O_RDWR | O_NOCTTY /*| O_NONBLOCK*/);
        if (-1 == *pfd)
        {
            status = BMSTATUS_INVALID;
            break;
        }
        tcgetattr(*pfd, &opt);
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
        tcsetattr(*pfd, TCSANOW, &opt);
    } while (0);
    return status;
}

BMStatus_t BMCommCtx_Open(BMCommCtx_pt ctx, BMCommConf_cpt conf)
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    do {
        if (BMSTATUS_SUCCESS != (status = Open(conf, &(ctx->fd))))
        {
            break;
        }
        if (BMSTATUS_SUCCESS != 
            (status = BMBaudDesc_ToSecPerByte(ctx->fd, &ctx->secPerByte)))
        {
            break;
        }
        ctx->rxrb = conf->rxrb;
        ctx->delay = conf->delay;
    } while (0);
    return status;
}

void BMCommCtx_Close(BMCommCtx_pt ctx)
{
    close(ctx->fd);
}