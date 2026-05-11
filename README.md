# RISC-V_Submission
# RISC-V ACT Framework Mentorship Challenge - UART Interface


This repository contains my solution for the LFX Mentorship coding challenge: configuring and communicating over a UART interface on Linux using the `termios` API.

## Implementation Details
The program (`uart_test.c`) is written in C and structured to handle low-level serial communication with robust error handling. 


* **Configuration:** Utilizes `tcgetattr` and `tcsetattr` to configure the port to `115200 8N1` (8 data bits, no parity, 1 stop bit). It explicitly disables canonical mode, echoing, and software/hardware flow control to ensure raw binary data transmission.
* **Transmission:** Uses `write()` to push a payload to the interface, checking for partial writes or device errors.
* **Timeout-based Reception:** Implements the `select()` system call to monitor the file descriptor. This provides a non-blocking 5-second timeout window to wait for incoming data before cleanly exiting.
* **Error Handling:** Evaluates `errno` on all system calls (`open`, `read`, `write`, `tcsetattr`, `select`) to provide descriptive stderr outputs, including specific hints for common permission issues (`EACCES`).

## Build Instructions
A standard GCC toolchain is required.
```bash
gcc -Wall -Wextra -O2 uart_test.c -o uart_test
