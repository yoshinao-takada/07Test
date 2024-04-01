#if !defined(BMCommCtx_H)
#define BMCommCtx_H
#include "BMTick.h"
#include "BMBuffer.h"
#define MIN_BLK_LEN 8

typedef struct {
    uint8_t devname[32]; // device name like "/dev/ttyUSB0"
    int bauddesc; // baudrate descriptor defined in termios.h
    BMRingBuffer_pt rxrb; // Rx ring buffer

    // A single shot delay timer is allocated to prohibit
    // transmission after it start to wait for reception.
    // refer to ../Docs/PHY-TxRx.md
    BMDispatcher_pt delay;
} BMCommConf_t, *BMCommConf_pt;
typedef const BMCommConf_t BMCommConf_cpt;

typedef struct {
    int fd; // file descriptor
    double secPerByte; // time period of a byte in seconds
    BMRingBuffer_pt rxrb; // Rx ring buffer
    BMBuffer_pt txb; // Tx linear buffer
    BMDispatcher_pt delay; // single shot delay
} BMCommCtx_t, *BMCommCtx_pt;

typedef struct {

} BMCommFSM_t, *BMCommFSM_pt;

#pragma region APIs
#pragma endregion APIs
#endif /* BMCommCtx_H */