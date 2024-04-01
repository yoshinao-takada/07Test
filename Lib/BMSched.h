#if !defined(BMSCHED_H)
#define BMSCHED_H
#include "BMBase.h"
/*!
\brief system initializer
*/
BMStatus_t BMSched_Init();

/*!
\brief scheduler main loop
*/
void BMSched_Main();
#endif /* BMSCHED_H */