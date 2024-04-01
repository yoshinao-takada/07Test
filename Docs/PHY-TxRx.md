# PHY of serial communication
## Overview
PHY is similar to UART ISR of MCUs.
A read thread is always running and usually blocked by read()
Unix API call. The thread is unblocked when a byte sequence arrives
at Rx port of the serialport.
The read thread reads the byte sequence, puts it into a ring buffer,
notify a data link layer of arrival of a byte sequence.
And then, the read thread starts a single shot delay to prohibit
transmission and calls read() again.
