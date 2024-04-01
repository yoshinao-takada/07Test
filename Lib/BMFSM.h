#if !defined(BMFSM_H)
#define BMFSM_H
#include "BMBase.h"
#define MAX_FSM_COUNT   8
typedef enum {
    BMStateResult_IGNORE,
    BMStateResult_ACCEPT,
    BMStateResult_TRANSIT,
    BMStateResult_ERR
} BMStateResult_t;

struct BMFSM;

/*!
\brief singly linked list element to manage BMFSM objects
*/
typedef struct BMFSMLink {
    struct BMFSMLink*  next;
    struct BMFSM* fsm;
} BMFSMLink_t, *BMFSMLink_pt;

/*!
\brief append an element to the link.
\param anchor [in] pointer to the 1st element.
    It is NULL when no element exists.
\param toappend [in] pointer to the element to append.
*/
void BMFSMLink_Append(BMFSMLink_pt anchor, BMFSMLink_pt toappend);

typedef struct BMFSM {
    // state
    BMStateResult_t (*state)(struct BMFSM* fsm, BMEv_pt ev);

    // closely related BMFSM objects
    BMFSMLink_pt relatives;

    // input event queue
    BMEvQ_pt evq;

    // context
    void* ctx;

    struct {
        // set downstream event queues
        BMStatus_t (*SetOutQ)(void* _ctx, uint16_t idx, BMEvQ_pt _evq);
        
        // 
    } APIs;
} BMFSM_t, *BMFSM_pt;

typedef BMFSM_t BMSched_t[MAX_FSM_COUNT];
#endif /* BMFSM_H */