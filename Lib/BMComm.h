#if !defined(BMCommRx_H)
#define BMCommRx_H
#include "BMBase.h"
#include "BMTick.h"
#include "BMBuffer.h"
#include "BMCRC.h"
#include "BMFSM.h"
#define MIN_BLK_LEN 8
#define MAX_RX_CH   4 /* multiple static buffers are declared. */
#define RXBUF_LEN   8

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
#pragma region BMComm
/*!
\brief serialport configuration struct.
*/
typedef struct {
    uint8_t devname[32]; // device name like "/dev/ttyUSB0"
    int bauddesc; // baudrate descriptor defined in termios.h
} BMCommConf_t, *BMCommConf_pt;
typedef const BMCommConf_t *BMCommConf_cpt;

typedef struct {
    int fd; // file descriptor
    double secPerByte; // 1 byte transfer time in seconds
} BMComm_t, *BMComm_pt;
typedef const BMComm_t *BMComm_cpt;

/*!
\brief open a serialport
\param conf [in] devicename and baudrate descriptor
\param pfd [out] file descriptor
\return result status
*/
BMStatus_t BMComm_Open(BMCommConf_cpt conf, BMComm_pt comm);

void BMComm_Close(BMComm_pt comm);
#pragma endregion BMComm
#pragma region BMCommRx
typedef struct {
    BMRingBuffer_pt rxrb; // Rx ring buffer

    // A single shot delay timer is allocated to prohibit
    // transmission after it start to wait for reception.
    // refer to ../Docs/PHY-TxRx.md
    BMDispatcher_pt delay;

    BMEvQ_pt oq; // input queue of downstream object
} BMCommRxConf_t, *BMCommRxConf_pt;
typedef const BMCommRxConf_t *BMCommRxConf_cpt;

typedef struct {
    BMComm_t base;
    BMRingBuffer_pt rxrb; // Rx ring buffer
    uint16_t delay_init;
    BMDispatcher_pt delay; // single shot delay
    BMEvQ_pt oq; // input queue of downstream object
    BMEv_t ev; // event to transfer to the downstream object
    pthread_t th; // read thread
    pthread_spinlock_t txdisable;
    int32_t quit_request;
    uint8_t rxbuf[RXBUF_LEN];
    uint16_t rxbuflen;
} BMCommRx_t, *BMCommRx_pt;

/*!
\brief Init a serialport receiver
\param conf [in] configuration info
\param fd [in] file descriptor of the serialport
\param Rx [out] serialport Rx context opened with the configuration.
*/
void
BMCommRx_Init(BMCommRx_pt Rx, BMCommRxConf_cpt conf, BMComm_cpt comm);

/*!
\brief start Rx thread
*/
BMStatus_t BMCommRx_Start(BMCommRx_pt Rx);

/*!
\brief stop Rx thread
*/
BMStatus_t BMCommRx_Stop(BMCommRx_pt Rx);

#pragma endregion BMCommRx

#pragma region BMCommTx
typedef struct {
    BMCRC_t crc;
    BMComm_pt comm;
    BMBufferQ_pt bufq;
} BMCommTxCtx_t, *BMCommTxCtx_pt;
typedef const BMCommTxCtx_t *BMCommTxCtx_cpt;

/*!
\brief handler of state, "Empty"
*/
BMStateResult_t BMCommTx_Empty(BMFSM_pt fsm, BMEv_pt ev);

/*!
\brief handler of state, "Remaining"
*/
BMStateResult_t BMCommTx_Remaining(BMFSM_pt fsm, BMEv_pt ev);
#pragma endregion BMCommTx
#endif /* BMCommRx_H */