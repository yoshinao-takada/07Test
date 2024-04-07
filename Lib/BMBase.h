#if !defined(BMBASE_H)
#define BMBASE_H
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <pthread.h>
#include <stddef.h>
#include <assert.h>
#include "BMEvId.h"

// Error log
#define BMERR_LOG(_file,_fn,_ln,...) \
    fprintf(stderr,__VA_ARGS__); \
    fprintf(stderr, "@ %s,%s,%d\n", _file, _fn, _ln)

#define BMERR_LOGBREAK(_file,_fn,_ln,...) \
    BMERR_LOG(_file,_fn,_ln,__VA_ARGS__); break

#define BMERR_LOGBREAKEX(...) \
        BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#define BMEND_FUNC(_file,_fn,_ln,_st) \
    fprintf(stdout, "Status = %d @ %s,%s,%d\n", _st,_file,_fn,_ln)

#define BMEND_FUNCEX(_st) \
    BMEND_FUNC(__FILE__, __FUNCTION__, __LINE__, _st)
    
// memory bourndary alignment suitable to architecture
#define ARCH_WORD_SIZE      8
#define ARCH_MASK           (ARCH_WORD_SIZE - 1)
#define DOWNALIGN_ARCH(_n)  ((_n) & (~ARCH_MASK))
#define ALIGN_ARCH(_n)      (DOWNALIGN_ARCH(_n) + ARCH_WORD_SIZE)

#if !defined(ARRAYSIZE)
#define ARRAYSIZE(_a) sizeof(_a)/sizeof(_a[0])
#endif

typedef int16_t BMStatus_t;
#define BMSTATUS_SUCCESS        (int16_t)EXIT_SUCCESS
#define BMSTATUS_COMMON         0x4100
// fatal error requiring to restart program.
#define BMSTATUS_FATAL          BMSTATUS_COMMON
#define BMSTATUS_TIMEOUT        (BMSTATUS_FATAL + 1)
#define BMSTATUS_CRCERROR       (BMSTATUS_FATAL + 2)
#define BMSTATUS_FRAMINGERROR   (BMSTATUS_FATAL + 3)
#define BMSTATUS_PARTIAL        (BMSTATUS_FATAL + 4)
#define BMSTATUS_COMPLETE       (BMSTATUS_FATAL + 5)
#define BMSTATUS_INVALID        (BMSTATUS_FATAL + 10)
#define BMSTATUS_SIZEMISMATCH   (BMSTATUS_FATAL + 11)
#define BMSTATUS_BUFFUL         (BMSTATUS_FATAL + 12)
#define BMSTATUS_NOTINIT        (BMSTATUS_FATAL + 13)

// base type of queue types.
typedef struct {
    uint16_t wridx, rdidx, count;
    pthread_spinlock_t lock;
} BMQBase_t, *BMQBase_pt;

#define BMQBase(_count) { 0u, 0u, _count, 0 }

#define BMQBase_LOCK(_qbase) pthread_spin_lock(&(_qbase.lock))
#define BMQBase_UNLOCK(_qbase) pthread_spin_unlock(&(_qbase.lock))

typedef uint16_t    BMId_t;

#pragma region Declare_BMEv_t
/*!
\brief Declare event type.
*/
typedef struct {
    // identifier of an event.
    uint16_t id;

    // `listeners` counts active listeners of the event.
    // Each listner decrement listeners when the listner releases the event.
    // The event can be recycled if listners == 0.
    uint16_t listeners;

    void* param;
} BMEv_t, *BMEv_pt;
#pragma endregion Declare_BMEv_t

#pragma region Declare_BMEvQ_t
/*!
\brief Event queue
*/
typedef struct {
    BMQBase_t qbase;
    BMEv_pt *events;
} BMEvQ_t, *BMEvQ_pt;

/*!
\brief declare BMEvQ_t variable as a local variable.
\param _varname [in] variable name
\param _size [in] buffer size in the queue
*/
#define BMEvQ_DECL(_varname, _size) \
BMEv_pt _varname ## _ev[_size]; \
BMEvQ_t _varname = { BMQBase(_size), _varname ## _ev }

#define BMEvQ_SDECL(_varname, _size) \
static BMEv_pt _varname ## _ev[_size]; \
static BMEvQ_t _varname = { BMQBase(_size), _varname ## _ev }

#define BMEvQ_INIT(_varname) \
pthread_spin_init(&(_varname.qbase.lock), PTHREAD_PROCESS_PRIVATE)

#define BMEvQ_DEINIT(_varname) \
pthread_spin_destroy(&(_varname.qbase.lock))

/*!
\brief declare an array of event queues.
\param _varname [in] array name
\param _size [in] buffer size in each queue
\param _count [in] array element count
*/
#define BMEvQ_ADECL(_varname, _size, _count) \
EMEv_pt _varname ## _ev[_size * _count]; \
BMEvQ_t _varname[_count];

#define BMEvQ_SADECL(_varname, _size, _count) \
static EMEv_pt _varname ## _ev[_size * _count]; \
static BMEvQ_t _varname[_count];

#define BMEvQ_AINIT(_varname) \
{ \
    int _count = ARRAYSIZE(_varname); \
    int _size = ARRAYSIZE(_varname ## _ev) / _count; \
    for (int _i = 0; _i < _count; _i++) \
    { \
        BMQBase_t _qbase = BMQBase(_size); \
        memcpy(&(_varname[i].qbase), &_qbase, sizeof(BMQBase_t)); \
        _varname[i].events = _varname ## _ev + _i * _size; \
        pthread_spin_init(&(_varname[i].qbase.lock), PTHREAD_PROCESS_PRIVATE); \
    } \
}

#define BMEvQ_ADEINIT(_varname) \
{ \
    int _count = ARRAYSIZE(_varname); \
    for (int _i = 0; _i < _count; _i++) \
    { \
        pthread_spin_destroy(&(_varname[i].qbase.lock)); \
    } \
}

/*!
\brief put an event pointer to the queue.
\param queue [in,out] queue object
\param pev [in] event pointer
\return number of event pointer which has been appended; i.e. 0 or 1.
*/
uint16_t BMEvQ_Put(BMEvQ_pt queue, BMEv_pt pev);

/*!
\brief get an event pointer from the queue.
\param queue [in,out] queue object
\param ppev [out] event pointer pointer to hold the retrieved event pointer
\return number of event pointer which has been retrieved.
*/
uint16_t BMEvQ_Get(BMEvQ_pt queue, BMEv_pt *ppev);

/*!
\brief Initialize an event queue.
*/
void BMEvQ_Init(BMEvQ_pt queue);

/*!
\brief Deinitialize an event queue.
*/
void BMEvQ_Deinit(BMEvQ_pt queue);
#pragma endregion Declare_BMEvQ_t

#pragma region Declare_BMEvPool_t
typedef struct {
    BMEv_pt ev; // array of events

    uint16_t count; // element count
    // each bit corresponds to each elements of ev.
    // 1: used, 0: available.
    uint16_t used;
    pthread_spinlock_t lock;
} BMEvPool_t, *BMEvPool_pt;

/*!
\brief Get an event.
\return NULL : no event is available, otherwise: one of available events.
*/
BMEv_pt BMEvPool_Get(BMEvPool_pt evpool);

/*!
\brief return an event to the pool.
*/
BMStatus_t BMEvPool_Return(BMEvPool_pt evpool, BMEv_pt ev);

/*!
\brief declare an instance of BMEvPool_t as a local variable.
*/
#define BMEvPool_DECL(_varname, _size) \
BMEv_t _varname ## _ev[_size]; \
BMEvPool_t _varname = { _varname ## _ev, _size, 0, 0 }

#define BMEvPool_SDECL(_varname, _size) \
static BMEv_t _varname ## _ev[_size]; \
static BMEvPool_t _varname = { _varname ## _ev, _size, 0, 0 }

#define BMEvPool_INIT(_varname) { \
    for (uint16_t i = 0; i < _varname.count; i++) \
    { \
        _varname.ev[i].param = NULL; \
        _varname.ev[i].id = _varname.ev[i].listeners = 0; \
    } \
    pthread_spin_init(&_varname.lock, PTHREAD_PROCESS_PRIVATE); \
}

#define BMEvPool_DEINIT(_varname) \
    pthread_spin_destroy(&_varname.lock)

#pragma endregion Declare_BMEvPool_t

#pragma region Declare_BMRingBuffer_t
/*!
\brief ring buffer for byte stream
*/
typedef struct {
    BMQBase_t qbase;
    uint8_t* bytes;
} BMRingBuffer_t, *BMRingBuffer_pt;

/*!
\brief declare BMRingBuffer_t variable as a local variable.
\param _varname [in] variable name
\param _size [in] buffer size in the queue
*/
#define BMRingBuffer_DECL(_varname, _size) \
uint8_t _varname ## _bytes[_size]; \
BMRingBuffer_t _varname = { BMQBase(_size), _varname ## _bytes }

#define BMRingBuffer_SDECL(_varname, _size) \
static uint8_t _varname ## _bytes[_size]; \
static BMRingBuffer_t _varname = { BMQBase(_size), _varname ## _bytes }

#define BMRingBuffer_INIT(_varname) \
pthread_spin_init(&(_varname.qbase.lock), PTHREAD_PROCESS_PRIVATE)

#define BMRingBuffer_DEINIT(_varname) \
pthread_spin_destroy(&(_varname.qbase.lock))

#define BMRingBuffer_ADECL(_varname, _size, _count) \
uint8_t _varname ## _bytes[_size * _count]; \
BMRingBuffer_t _varname[_count]

#define BMRingBuffer_SADECL(_varname, _size, _count) \
static uint8_t _varname ## _bytes[_size * _count]; \
static BMRingBuffer_t _varname[_count]

#define BMRingBuffer_AINIT(_varname) \
{ \
    int _count = ARRAYSIZE(_varname); \
    int _size = ARRAYSIZE(_varname ## _bytes) / _count; \
    for (int _i = 0; _i < _count; _i++) \
    { \
        BMQBase_t _qbase = BMQBase(_size); \
        memcpy(&(_varname[_i].qbase), &_qbase, sizeof(BMQBase_t)); \
        _varname[_i].bytes = _varname ## _bytes + _i * _size; \
        pthread_spin_init(&(_varname[_i].qbase.lock), PTHREAD_PROCESS_PRIVATE); \
    } \
}

#define BMRingBuffer_ADEINIT(_varname) \
{ \
    int _count = ARRAYSIZE(_varname); \
    int _size = ARRAYSIZE(_varname ## _bytes) / _count; \
    for (int _i = 0; _i < _count; _i++) \
    { \
        pthread_spin_destroy(&(_varname[_i].qbase.lock)); \
    } \
}

/*!
\brief put a byte to the ring buffer
\param rb [in,out] ring buffer object
\param byte [in] pointer to the byte to put
\return number of byte which has been appended; i.e. 0 or 1
*/
uint16_t BMRingBuffer_Put(BMRingBuffer_pt rb, const uint8_t* byte);

/*!
\brief get a byte from the ring buffer
\param rb [in,out] ring buffer object
\param byte [out] byte buffer to retrieve a byte into
\return number of byte which has been retrieved; i.e. 0 or 1
*/
uint16_t BMRingBuffer_Get(BMRingBuffer_pt rb, uint8_t* byte);

/*!
\brief put bytes to the ring buffer
\param rb [in,out] ring buffer object
\param bytes [in] pointer to the bytes to put
\param count [in] size of bytes
\return number of byte which has been appended; i.e. 0..count
*/
uint16_t BMRingBuffer_Puts
(BMRingBuffer_pt rb, const uint8_t* bytes, uint16_t count);

/*!
\brief get bytes from the ring buffer
\param rb [in,out] ring buffer object
\param bytes [in] pointer to the byte buffer to retrieve bytes into
\param count [in] size of bytes
\return number of byte which has been appended; i.e. 0..count
*/
uint16_t BMRingBuffer_Gets
(BMRingBuffer_pt rb, uint8_t* bytes, uint16_t count);
#pragma endregion Declare_BMRingBuffer_t

#endif /* BMBASE_H */