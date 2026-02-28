// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Robert Tiismus

/*
* CASIO.H
*
* Header file for menu driven communication utility for transmitting data between 
* Casio fx-7700GB Program-Link serial interface and MS-DOS PC LPT interface
*/

// LPT1 port address

#define LPT     0x379

// Error conditions returned by getchr() function

#define TIMEOUT         -1
#define PARITY_ERR      -2
#define USER_BREAK      -3

// Parity definitions

typedef enum { PARITY_EVEN = 0, PARITY_ODD = 1, PARITY_NONE = 2 } party;

#define TIMER_INT       0x08    // Timer interrupt number
#define KEYBOARD_INT    0x09    // Keyboard interrupt number

                /* Communication messages */

#define ACK     0x06    // Message: Acknowledgement
#define DC3     0x13    // Message: Dev control 3 (User defined: Clear to send)
#define NAC     0x15    // Message: Negative acknowledgement
#define SYN     0x16    // Message: Synchronous idle ( Request to send )

#define AU      0x21    // ! Warning: Area used

#define RER     0x22    // " Error: Generic error
#define CER     0x2B    // + Error: Checksum error
#define MF      0x24    // $ Error: Memory full
#define IMO     0x3F    // ? Error: Incorrect mode

#define DIC     0x3A    // : Identifier: Data is coming
#define HDR     0x50    // P Identifier: Header

#define ONE     0x31    // 1 Mode: One program transfer
#define ALL     0x5A    // Z Mode: All programs transfer

#define EOP     0xFF    // Marker: End of program

// Timeout values used by getchr() function

#define seconds     * 1
#define minutes     * 60

#define SHORTWAIT       1 seconds
#define LONGWAIT        6 minutes

// System timer input Tic = 1.1931817 MHz
#define Tic     1193182

// Baud rates Baud = 1200 | 2400 | 4800 | 9600 bps ( [bps] equals [Hz] )
// Divisor = Tic / Baud  ( see below )

#define b1200   Tic / 1200      //0x3E2
#define b2400   Tic / 2400      //0x1F1
#define b4800   Tic / 4800      //0xF8
#define b9600   Tic / 9600      //0x7C
