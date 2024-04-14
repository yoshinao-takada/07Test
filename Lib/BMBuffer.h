#if !defined(BMBUFFER_H)
#define BMBUFFER_H
#include "BMBase.h"
#if !defined(BMBUFFERPOOL_LONGBUFFERCOUNT)
#define BMBUFFERPOOL_LONGBUFFERCOUNT    16
#endif
#if !defined(BMBUFFERPOOL_SHORTBUFFERCOUNT)
#define BMBUFFERPOOL_SHORTBUFFERCOUNT   8
#endif
#if !defined(BMBUFFERPOOL_LONGBUFFERSIZE)
#define BMBUFFERPOOL_LONGBUFFERSIZE    64
#endif
#if !defined(BMBUFFERPOOL_SHORTBUFFERSIZE)
#define BMBUFFERPOOL_SHORTBUFFERSIZE   16
#endif

#pragma region DECLARE_BMBuffer_t
typedef struct {
    uint8_t *buf;
    uint16_t size, filled, crunched;
} BMBuffer_t, *BMBuffer_pt;
typedef const BMBuffer_t *BMBuffer_cpt;

/*!
\brief Clear the buffer.
*/
#define BMBuffer_CLEAR(_buf) (_buf).filled = (_buf).crunched = 0

/*!
\brief Get the pointer pointing the byte to write.
*/
#define BMBuffer_WRPTR(_buf) ((_buf).buffer + (_buf).filled)

#define BMBuffer_RDPTR(_buf) ((_buf).buffer + (_buf).crunched)

/*!
\brief Available byte count in the buffer.
*/
#define BMBuffer_AVAILABLE(_buf) ((_buf).size - (_buf).filled)

#define BMBuffer_DECL(_varname, _size) \
    uint8_t _varname ## _buf[_size]; \
    BMBuffer_t _varname = { _varname ## _buf, _size, 0, 0 }
#pragma endregion DECLARE_BMBuffer_t

#pragma region DECLARE_BMBufferQ_t
typedef struct {
    BMQBase_t qbase;
    BMBuffer_pt *buffers;
} BMBufferQ_t, *BMBufferQ_pt;

#define BMBufferQ_DECL(_varname, _size) \
    BMBuffer_pt _varname ## _buffers[_size]; \
    BMBufferQ_t _varname = { BMQBase(_size), _varname ## _buffers }

/*!
\brief initialize an spinlock in a buffer.
*/
#define BMBufferQ_INIT(_varname) \
    pthread_spin_init(&((_varname).qbase.lock), PTHREAD_PROCESS_PRIVATE)

/*!
\brief release resources allocated to a spinlock in a buffer.
*/
#define BMBufferQ_DEINIT(_varname) \
    pthread_spin_destroy(&((_varname).qbase.lock))

/*!
\brief put a buffer into a queue
*/
uint16_t BMBufferQ_Put(BMBufferQ_pt q, BMBuffer_pt buffer);

/*!
\brief get a buffer from a queue.
*/
uint16_t BMBufferQ_Get(BMBufferQ_pt q, BMBuffer_pt *ppbuffer);

/*!
\brief peek the first content in the queue.
*/
BMBuffer_pt BMBufferQ_Peek(BMBufferQ_pt q);
#pragma endregion DECLARE_BMBufferQ_t

#pragma region DECLARE_BMBufferPool_t
typedef struct {
    BMBuffer_pt buffers;
    pthread_spinlock_t lock;
    uint16_t used; // each bit corresponds to each element of buffers.
                    // 1: used, 0: available.
    uint16_t size;
} BMBufferPool_t, *BMBufferPool_pt;
typedef const BMBufferPool_t *BMBufferPool_cpt;

/*!
\brief Declare a buffer pool instance as a local variable.
\param _varname [in] variable name of the buffer pool
\param _poolsize [in] number of buffers in the pool
\param _bufsize [in] buffer size of each buffer in the pool
*/
#define BMBufferPool_DECL(_varname, _poolsize, _bufsize) \
    uint8_t _varname ## _bufferbody[_poolsize * _bufsize]; \
    BMBuffer_t _varname ## _buffers[_poolsize] = { \
        { _varname ## _bufferbody, _bufsize, 0 }, \
    }; \
    BMBufferPool_t _varname = { \
        _varname ## _buffers, 0, 0, _poolsize }

/*!
\brief Declare a buffer pool instance as a static variable.
*/
#define BMBufferPool_SDECL(_varname, _poolsize, _bufsize) \
    static uint8_t _varname ## _bufferbody[_poolsize * _bufsize]; \
    static BMBuffer_t _varname ## _buffers[_poolsize] = { \
        { _varname ## _bufferbody, _bufsize, 0 }, \
    }; \
    static BMBufferPool_t _varname = { \
        _varname ## _buffers, 0, 0, _poolsize }

/*!
\brief Initialize a buffer pool declared by BMBufferPool_DECL
    or BMBufferPool_SDECL.
*/
#define BMBufferPool_INIT(_varname) { \
    uint16_t _bufsize = _varname.buffers[0].size; \
    pthread_spin_init(&_varname.lock, PTHREAD_PROCESS_PRIVATE); \
    for (uint16_t i = 0; i < _varname.size; i++) { \
        _varname.buffers[i].buf = (_varname ## _bufferbody) + i * _bufsize; \
        _varname.buffers[i].size = _bufsize; \
        _varname.buffers[i].filled = _varname.buffers[i].crunched = 0; \
    } \
}

/*!
\brief Deinitialize a buffer pool.
*/
#define BMBufferPool_DEINIT(_varname) \
    pthread_spin_destroy(&_varname.lock)

/*!
\brief lock the buffer pool with spinlock.
*/
#define BMBufferPool_LOCK(_varname) \
    pthread_spin_lock(&_varname.lock)

/*!
\brief unlock the buffer pool with spinlock.
*/
#define BMBufferPool_UNLOCK(_varname) \
    pthread_spin_unlock(&_varname.lock)

/*!
\brief get a buffer
\return NULL: no available one. otherwise: one of available one.
*/
BMBuffer_pt BMBufferPool_Get(BMBufferPool_pt bpl);

/*!
\brief return a buffer.
*/
BMStatus_t BMBufferPool_Return(BMBufferPool_pt bpl, BMBuffer_pt buffer);

typedef enum {
    BMBufferPoolType_LONG,
    BMBufferPoolType_SHORT,
} BMBufferPoolType_t;

BMBuffer_pt BMBufferPool_SGet(BMBufferPoolType_t type);
BMStatus_t BMBufferPool_SReturn(BMBuffer_pt buffer);
void BMBufferPool_SInit();
void BMBufferPool_SDeinit();
#pragma endregion DECLARE_BMBufferPool_t
#endif /* BMBUFFER_H */