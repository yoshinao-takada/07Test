#if !defined(BMFSM_H)
#define BMFSM_H
#include "BMBase.h"
#define MAX_FSM_COUNT   8
#define MAX_DOWNSTREAM_QUEUES   4

typedef enum {
    BMStateResult_IGNORE,
    BMStateResult_ACCEPT,
    BMStateResult_TRANSIT,
    BMStateResult_ERR
} BMStateResult_t;

typedef struct BMFSM {
    // state
    BMStateResult_t (*state)(struct BMFSM* fsm, BMEv_pt ev);

    // input event queue
    BMEvQ_pt iq;

    // input event queues of downstream objects
    BMEvQ_pt oq[MAX_DOWNSTREAM_QUEUES];

    // context can hold a variable of any type.
    void* ctx;
} BMFSM_t, *BMFSM_pt;

typedef BMFSM_t BMSched_t[MAX_FSM_COUNT];
#endif /* BMFSM_H */