# Finite State Machine for Small Embedded Systems
BMFSM (Bare Metal Finite State Machine) was inspired by QP nano of Quantum Leaps.
Each state is represented by a function pointer of a state handler.
A state handler implements two parts.
1) Decision maker of transition selects a next state.
2) `Do` and `Exit` action handlers correspond to `Do` and `Exit` operator in
UML state diagram.  
Note: `Entry` handler is moved to `Exit` handlers in previous states.</br>

These ideas made a state handlers simpler than those of QP.

Every states are driven by events sent by other FSMs or ISRs.
The events are represented by a union containing an event identifier or
a pointer to an event object as shown below.
```
// General purpose identifier type is declared as a type
// suitable to a platform archtecture.
typedef uint16_t BMId_t;

typedef union {
    void* ptr;
    BMId_t id;
} BMEvId_t, *BMEvId_pt;

typedef struct {
    BMEvId_t id;
    // listners counts active listners of the event.
    // Each listner decrement listeners when the listner releases the event.
    // The event can be recycled if listners == 0.
    uint16_t listeners;
} BMEv_t, *BMEv_pt;
```

Events can be saved in a event queue. The queue is defined as shown below.
```
// base type of queue types
// write index, read index and element count of a buffer.
// Actually available space is (count - 1).
typedef struct {
    uint16_t wridx, rdidx, count;
} BMQBase_t, *BMQBase_pt;

typedef struct {
    BMEv_pt events;
    BMQBase_t qbase;
} BMEvQ_t, *BMEvQ_pt;

typedef struct {
    uint8_t* bytes;
    BMQBase_t qbase;
} BMRingBuffer_t, *BMRingBuffer_pt;
```

BMFSM is declared as shown below.
```
typedef enum {
    BMStateResult_IGNORE,
    BMStateResult_ACCEPT,
    BMStateResult_TRANSIT,
    BMStateResult_ERR,
} BMStateResult_t;

struct BMFSM;

typedef struct BMFSMLink {
    struct BMFSMLink* next;
    struct BMFSM* fsm;
} BMFSMLink_t, *BMFSMLink_pt;

typedef struct BMFSM {
    // state, which is actually a state handler
    BMStateResult_t (*State)(struct BMFSM* fsm, BMEv_pt ev);

    // closely related BMFSM objects.
    BMFSMLink_pt relatives;

    // event queue
    BMEvQ_t evq;
} BMFSM_t, *BMFSM_pt;
```

Each thread has a scheduler which manage multiple FSMs.
Usually a small MCU system has only one thread.
```
typedef BMSched_t   BMFSM_pt[MAX_FSM_COUNT];
```

An ISR can usually manage multiple devices. Therefore an ISR can see
an array of aggregated properties of multiple devices.
For example, considering POSIX serialport, a device property consists of
```
#define COMMBUF_SIZE        8
typedef struct {
    BMRingBuffer_pt rb;
    int fd;
} BMCommProps_t, *BMCommProps_pt;
```