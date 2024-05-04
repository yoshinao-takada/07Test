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
#include <signal.h>
#include "BMEvId.h"

// Error log
#if defined(BMBASE_C)
pthread_spinlock_t baselock;
#else
extern pthread_spinlock_t baselock;
#endif
#define BMBASELOCK_INIT pthread_spin_init(&baselock, PTHREAD_PROCESS_PRIVATE)
#define BMBASELOCK_DESTROY pthread_spin_destroy(&baselock)
#define BMLOCKED_PRINTF(...) \
    pthread_spin_lock(&baselock); \
    printf(__VA_ARGS__); \
    pthread_spin_unlock(&baselock)

#define BMERR_LOG(_file,_fn,_ln,...) \
    pthread_spin_lock(&baselock); \
    fprintf(stderr,__VA_ARGS__); \
    fprintf(stderr, "@ %s,%s,%d\n", _file, _fn, _ln); \
    pthread_spin_unlock(&baselock)

#define BMERR_LOGBREAK(_file,_fn,_ln,...) \
    BMERR_LOG(_file,_fn,_ln,__VA_ARGS__); break

#define BMERR_LOGBREAKEX(...) \
        BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#define BMEND_FUNC(_file,_fn,_ln,_st) \
    BMLOCKED_PRINTF("Status = %d @ %s,%s,%d\n", _st,_file,_fn,_ln)

#define BMEND_FUNCEX(_st) \
    BMEND_FUNC(__FILE__, __FUNCTION__, __LINE__, _st)
    
// memory bourndary alignment suitable to architecture
#define ARCH_WORD_SIZE      8
#define ARCH_MASK           (ARCH_WORD_SIZE - 1)
#define DOWNALIGN_ARCH(_n)  ((_n) & (~ARCH_MASK))
#define ALIGN_ARCH(_n)      (DOWNALIGN_ARCH(_n) + ARCH_WORD_SIZE)

#define BMEVQPOOL_QCOUINT   8
#define BMEVQPOOL_QSIZE     4

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

uint16_t BMQBase_NextWrIdx(BMQBase_pt q);
uint16_t BMQBase_NextRdIdx(BMQBase_pt q);
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
BMEv_pt _varname ## _ev[_size * _count]; \
BMEvQ_t _varname[_count];

#define BMEvQ_SADECL(_varname, _size, _count) \
static BMEv_pt _varname ## _ev[_size * _count]; \
static BMEvQ_t _varname[_count];

#define BMEvQ_AINIT(_varname) \
{ \
    int _count = ARRAYSIZE(_varname); \
    int _size = ARRAYSIZE(_varname ## _ev) / _count; \
    for (int _i = 0; _i < _count; _i++) \
    { \
        BMQBase_t _qbase = BMQBase(_size); \
        memcpy(&(_varname[_i].qbase), &_qbase, sizeof(BMQBase_t)); \
        _varname[_i].events = _varname ## _ev + _i * _size; \
        pthread_spin_init(&(_varname[_i].qbase.lock), PTHREAD_PROCESS_PRIVATE); \
    } \
}

#define BMEvQ_ADEINIT(_varname) \
{ \
    int _count = ARRAYSIZE(_varname); \
    for (int _i = 0; _i < _count; _i++) \
    { \
        pthread_spin_destroy(&(_varname[_i].qbase.lock)); \
    } \
}

/*!
\brief initialize static pool of BMEvQ_t
*/
void BMEvQPool_SInit();

/*!
\brief deinitialize static pool of BMEvQ_t
*/
void BMEvQPool_SDeinit();

/*!
\brief get an instance in static pool of BMEvQ_t.
*/
BMEvQ_pt BMEvQPool_SGet();

/*!
\brief return the instance obtained by BMEvQPool_SGet() to the
    static pool.
*/
BMStatus_t BMEvQPool_SReturn(BMEvQ_pt q);

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

/*!
\brief find the first available bit number.
    0 bits in x are available and 1 bits in x are used.
\param x [in] bit flags showing available or not.
\param count [in] effective bit count in x. if (count < 16), larger number bits
    are ineffective.
\return -1: no available bit found, number >= 0: available bit number
*/
int16_t BMPoolSupport_FindAvailable(uint16_t *x, uint16_t count);
#endif /* BMBASE_H */