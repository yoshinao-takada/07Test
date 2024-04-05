#if !defined(BMCommCtx_H)
#define BMCommCtx_H
#include "BMBase.h"
#include "BMTick.h"
#include "BMBuffer.h"
#include "BMFSM.h"
#define MIN_BLK_LEN 8
#pragma region BAUDRATE_UTILITY
/*!
\brief convert baudrate descriptor to baudrate (bit/sec)
*/
int BMBaudDesc_ToBaudrate(int bauddesc);

/*!
\brief convert baudrate to baudrate descriptor.
*/
int BMBaudDesc_FromBaudrate(int baudrate);

/*!
\brief Get seconds required to transfer a byte.
*/
BMStatus_t
BMBaudDesc_ToSecPerByte(int fd, double* secPerByte);
#pragma endregion BAUDRATE_UTILITY

/*!
\brief serialport configuration struct.
*/
typedef struct {
    uint8_t devname[32]; // device name like "/dev/ttyUSB0"
    int bauddesc; // baudrate descriptor defined in termios.h
    BMRingBuffer_pt rxrb; // Rx ring buffer

    // A single shot delay timer is allocated to prohibit
    // transmission after it start to wait for reception.
    // refer to ../Docs/PHY-TxRx.md
    BMDispatcher_pt delay;
} BMCommConf_t, *BMCommConf_pt;
typedef const BMCommConf_t *BMCommConf_cpt;

typedef struct {
    int fd; // file descriptor
    double secPerByte; // time period of a byte in seconds
    BMRingBuffer_pt rxrb; // Rx ring buffer
    BMBuffer_pt txb; // Tx linear buffer
    BMDispatcher_pt delay; // single shot delay
} BMCommCtx_t, *BMCommCtx_pt;

/*!
\brief Open a serialport.
\param conf [in] configuration info
\param ctx [out] serialport context opened with the configuration.
\return status code
*/
BMStatus_t BMCommCtx_Open(BMCommCtx_pt ctx, BMCommConf_cpt conf);

void BMCommCtx_Close(BMCommCtx_pt ctx);

typedef struct {

} BMCommFSM_t, *BMCommFSM_pt;

#pragma region APIs
#pragma endregion APIs
#endif /* BMCommCtx_H */