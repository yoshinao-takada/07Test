#if !defined(BMSCHED_H)
#define BMSCHED_H
#include "BMBase.h"
#include "BMFSM.h"
#include "BMTick.h"
#define BMSCHED_SIZE    16

typedef struct {
    uint16_t size;
    BMFSM_pt *fsms;
} BMSched_t, *BMSched_pt;

#define BMSched_DECL(_varname, _size) \
    BMFSM_pt _varname ## _fsms[_size]; \
    BMSched_t _varname = { _size, _varname ## _fsms }

#define BMSched_SDECL(_varname, _size) \
    static BMFSM_pt _varname ## _fsms[_size]; \
    static BMSched_t _varname = { _size, _varname ## _fsms }

/*!
\brief system initializer
*/
BMStatus_t BMSched_Init(BMSched_pt sched);

/*!
\brief Get the index of the 1st available element of fsms.
*/
int16_t BMSched_1stAvailable(BMSched_pt sched);

/*!
\brief scheduler main loop
*/
void BMSched_Main(BMSched_pt sched);
#endif /* BMSCHED_H */