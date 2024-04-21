#if !defined(BMRINGBUFFER_H)
#define BMRINGBUFFER_H
#include "BMBase.h"
#if !defined(BMRINGBUFFERPOOL_LONGBUFFERCOUNT)
#define BMRINGBUFFERPOOL_LONGBUFFERCOUNT    4
#endif
#if !defined(BMRINGBUFFERPOOL_SHORTBUFFERCOUNT)
#define BMRINGBUFFERPOOL_SHORTBUFFERCOUNT   4
#endif
#if !defined(BMRINGBUFFERPOOL_LONGBUFFERSIZE)
#define BMRINGBUFFERPOOL_LONGBUFFERSIZE    256
#endif
#if !defined(BMRINGBUFFERPOOL_SHORTBUFFERSIZE)
#define BMRINGBUFFERPOOL_SHORTBUFFERSIZE   16
#endif

#pragma region Declare_BMRingBuffer_t
/*!
\brief ring buffer for byte stream
*/
typedef struct {
    BMQBase_t base;
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
pthread_spin_init(&(_varname.base.lock), PTHREAD_PROCESS_PRIVATE)

#define BMRingBuffer_DEINIT(_varname) \
pthread_spin_destroy(&(_varname.base.lock))

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
        memcpy(&(_varname[_i].base), &_qbase, sizeof(BMQBase_t)); \
        _varname[_i].bytes = _varname ## _bytes + _i * _size; \
        pthread_spin_init(&(_varname[_i].base.lock), PTHREAD_PROCESS_PRIVATE); \
    } \
}

#define BMRingBuffer_ADEINIT(_varname) \
{ \
    int _count = ARRAYSIZE(_varname); \
    int _size = ARRAYSIZE(_varname ## _bytes) / _count; \
    for (int _i = 0; _i < _count; _i++) \
    { \
        pthread_spin_destroy(&(_varname[_i].base.lock)); \
    } \
}

#define BMRingBuffer_ISEMPTY(_varptr) \
    (_varptr->base.wridx == _varptr->base.rdidx)
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

#pragma region Declare_BMRingBufferPool_t
typedef struct {
    BMRingBuffer_pt buffers;
    pthread_spinlock_t lock;
    uint16_t used;
    uint16_t size; // pool size
} BMRingBufferPool_t, *BMRingBufferPool_pt;

#define BMRingBufferPool_DECL(_varname, _poolsize, _bufsize) \
    uint8_t _varname ## _bufferbody[_poolsize * _bufsize]; \
    BMRingBuffer_t _varname ## _buffers[_poolsize] = { \
        { BMQBase(_bufsize), _varname ## _bufferbody } }; \
    BMRingBufferPool_t _varname = { \
        _varname ## _buffers, 0, 0, _poolsize }

#define BMRingBufferPool_SDECL(_varname, _poolsize, _bufsize) \
    static uint8_t _varname ## _bufferbody[_poolsize * _bufsize]; \
    static BMRingBuffer_t _varname ## _buffers[_poolsize] = { \
        { BMQBase(_bufsize), _varname ## _bufferbody } }; \
    static BMRingBufferPool_t _varname = { \
        _varname ## _buffers, 0, 0, _poolsize }

#define BMRingBufferPool_INIT(_varname) { \
    uint16_t _bufsize = _varname.buffers[0].base.count; \
    BMQBase_t _qbase = BMQBase(_bufsize); \
    uint8_t* body = _varname ## _bufferbody; \
    BMRingBuffer_pt _buffers = _varname.buffers; \
    /* init pool's lock object */ \
    pthread_spin_init(&_varname.lock, PTHREAD_PROCESS_PRIVATE); \
    for (uint16_t _i = 0; _i < _varname.size; _i++, _buffers++) { \
        /* init each ringbuffer */ \
        memcpy(_buffers, &_qbase, sizeof(BMQBase_t)); \
        pthread_spin_init(&_buffers->base.lock, PTHREAD_PROCESS_PRIVATE); \
        _buffers->bytes = _varname ## _bufferbody + _i * _bufsize; \
    } \
}

#define BMRingBufferPool_DEINIT(_varname) { \
    BMRingBuffer_pt _buffers = _varname.buffers; \
    pthread_spin_destroy(&_varname.lock); \
    for (uint16_t _i = 0; _i < _varname.size; _i++, _buffers++) { \
        pthread_spin_destroy(&_buffers->base.lock); \
    } \
}

/*!
\brief get a buffer
\return NULL: no available one, otherwise: one of available one.
*/
BMRingBuffer_pt BMRingBufferPool_Get(BMRingBufferPool_pt pool);

/*!
\brief return a buffer
*/
BMStatus_t BMRingBufferPool_Return
(BMRingBufferPool_pt pool, BMRingBuffer_pt buffer);

typedef enum {
    BMRingBufferPoolType_LONG,
    BMRingBufferPoolType_SHORT,
} BMRingBufferPoolType_t;

BMRingBuffer_pt BMRingBufferPool_SGet(BMRingBufferPoolType_t type);
BMStatus_t BMRingBufferPool_SReturn(BMRingBuffer_pt rb);
void BMRingBufferPool_SInit();
void BMRingBufferPool_SDeinit();
#pragma endregion Declare_BMRingBufferPool_t
#endif /* BMRINGBUFFER_H */