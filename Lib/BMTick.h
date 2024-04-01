#if !defined(BMTICK_H)
#define BMTICK_H
#include "BMBase.h"
#pragma region DISPATCHERS
typedef struct {
    uint32_t count, init;
    void* (*handler)(void*);
    void* param;
    void* result;
} BMDispatcher_t, *BMDispatcher_pt;

void BMDispatcher_Set(BMDispatcher_pt dispatcher,
    uint32_t count, uint32_t init,
    void* (*_handler)(void*), void* param);

void BMDispatcher_Reset(BMDispatcher_pt dispatcher);

BMStatus_t BMDispatcher_Dispatch(BMDispatcher_pt dispatcher);

typedef struct {
    uint16_t count;
    BMDispatcher_pt dispatchers;
    BMEvQ_pt q;
} BMDispatchers_t, *BMDispatchers_pt;

/*!
\brief declare BMDispatchers_t instance as
_varname = { _size, _varname_dispatchers[_size], &(_varname_Q), };
\param _varname [in] variable name of BMDispatcher_t instance.
\param _size [in] dispatcher element count
*/
#define BMDISPATCHERS_QUEUESIZE 2
#define BMDispatchers_DECL(_varname, _size) \
    BMEvQ_DECL(_varname ## _Q, BMDISPATCHERS_QUEUESIZE); \
    BMDispatcher_t _varname ## _dispatchers[_size]; \
    BMDispatchers_t _varname = \
        { _size, _varname ## _dispatchers, &(_varname ## _Q) };

#define BMDispatchers_INIT(_varname) BMEvQ_INIT(_varname ## _Q)

#define BMDispatchers_DEINIT(_varname) BMDispatchers_Clear(&(_varname)); \
    BMEvQ_DEINIT(_varname ## _Q)
/*!
\brief clear all the elements in dispatchers.
*/
void BMDispatchers_Clear(BMDispatchers_pt dispatchers);

/*!
\brief dispatch with all dispatchers.
*/
BMStatus_t BMDispatchers_Dispatch(BMDispatchers_pt dispatchers);

/*!
\brief Invoke dispatching if more than 0 events are ready in the queue.
*/
BMStatus_t BMDispatchers_CrunchEvent(BMDispatchers_pt dispatchers);
#pragma endregion DISPATCHERS

#pragma region SYSTICK_TIMER
/*!
\brief Initialize systick timer.
\param evq [in] event queue of an FSM implementing BMDispatchers_t
    which dispatch systick overflow events by decimating original frequency.
\param period [in] period in milliseconds
*/
BMStatus_t BMSystick_Init(BMEvQ_pt evq, uint16_t period);

/*!
\brief Deinitialize systick timer.
*/
BMStatus_t BMSystick_Deinit();
#pragma endregion SYSTICK_TIMER
#endif /* BMTICK_H */