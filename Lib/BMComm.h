#if !defined(BMCOMM_H)
#define BMCOMM_H
#include "BMBase.h"
#include "BMFSM.h"
#include "BMBuffer.h"
#include "BMRingBuffer.h"
#include "BMCRC.h"
#include "BMTick.h"
#define BMCOMM_MAX_CH   4
#define BMCOMM_RAW_BUFSIZE  16

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
// BMComm_t manage a file descriptor and its byte time perild.
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

#pragma region BMCommThCtx
/*!
\brief thread parameter of Rx thread.
Rx thread dose
1) prohibit Tx until prohibit timer timeout
2) blocking read()
3A) puts Rx chars into Rx Ringbuffer
3B) prohibit timeout cancels the prohibition.
4A) notify PHY FSM of NOT empty Rx Ringbuffer
5) goto 1)
Participant in the acitvity are
1> Ring buffer
2> one-shot delay timer
3> exclusive control object to control Tx prohibition
4> serialport file descriptor and byte time
5> event queue of PHY FSM
*/
typedef struct {
    BMComm_t base; // serialport descriptor
    void* (*thstart)(void*); // thread function
    pthread_t th; // thread
    pthread_mutex_t* rbblock; // blocked by rb status
    int cont; // flag to continue to thread main loop
    BMEvQ_pt evq; // event queue of PHY FSM
    BMEv_t ev; // event real body sent to PHY FSM
    BMRingBuffer_pt rb; // ringbuffer shared with PHY FSM
    BMBuffer_pt buffer; // raw Rx buffer which thread func uses.
    BMStatus_t status; // thread result status
} BMCommThCtx_t, *BMCommThCtx_pt;

typedef struct {
    BMCommThCtx_t base;
    int* wrproh;
    BMDispatcher_pt oneshot; // 1-shot delay timer to prohibit write operation
} BMCommRxThCtx_t, *BMCommRxThCtx_pt;

BMStatus_t BMCommThCtx_Init(BMCommThCtx_pt ctx, BMComm_cpt comm,
    pthread_mutex_t* rbblock, BMEvQ_pt evq);

BMStatus_t BMCommRxThCtx_Init(BMCommRxThCtx_pt ctx, BMComm_cpt comm,
    int* wrproh, pthread_mutex_t* rbblock, BMEvQ_pt evq,
    BMDispatcher_pt oneshot);
    
/*!
\brief Rx thread function
*/
void* BMComm_RxTh(void* ctx);

/*!
\brief Tx thread function
*/
void* BMComm_TxTh(void* ctx);
#pragma endregion BMCommThCtx

typedef struct {
    BMCommRxThCtx_t rxctx;
    BMCommThCtx_t txctx;
    int wrproh;
    pthread_mutex_t rbblock; // locked during Tx RB being empty.
    BMEvQ_pt dllQ; // datalink layer input queue
} BMCommCtx_t, *BMCommCtx_pt;


/*!
\brief init FSM context
*/
BMStatus_t BMCommCtx_Init
(BMCommCtx_pt ctx, BMComm_cpt comm, BMDispatchers_pt dispather,
 BMEvQ_pt phyQ);

void BMCommCtx_Deinit(BMCommCtx_pt ctx);

// reading state
BMStateResult_t BMCommFSM_Read(BMFSM_pt fsm, BMEv_pt ev);

// read & pause state
BMStateResult_t BMCommFSM_ReadPause(BMFSM_pt fsm, BMEv_pt ev);

// writing state
BMStateResult_t BMCommFSM_Write(BMFSM_pt fsm, BMEv_pt ev);
#endif /* BMCOMM_H */