# Names of library sources
## Lib
### BMBase.h
It declares common types and macros.

* BMQBase_t : base type of BMEvQ_t and BMRingBuffer_t.
* BMEvQ_t : Event queue
* BMRingBuffer_t : ring buffer for byte stream

### BMFSM.h
It declares finite statemachine type and scheduler
to invoke array of finite statemachines.

### BMCRC.h
It declares CRC endec type.

### BMDebug.h
It declares debugging aids.

### BMComm.h
It declares serialport communication functions.
* BMCommConf_t : configuratioin of serialport
* BMCommBuffer_t : packet buffer shared by network layers
* BMBufPool_t : buffer pool for protocol stack
* BMComm_t : reader/writer of serialports. reader has its own thread.

### BMTick.h
* SIGALRM handler/setup/cleanup
* Dispatcher of timer event

