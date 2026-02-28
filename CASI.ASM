; SPDX-License-Identifier: GPL-3.0-or-later
; Copyright (C) 2026 Robert Tiismus

; CASI.ASM
;
; LPT port RS-232 TTL-5V driver for menu driven communication utility for
; transmitting data between Casio fx-7700GB Program-Link serial interface and
; MS-DOS PC LPT interface

        .386

        title   casi

        LOCALS

INCLUDE casio.ash

; Remove comment from front of the line below if you want to hear modem-like
; noise at the time of transmissions:
;NOISE   equ     1

waitint MACRO
        local   wi
        xor     al,al
wi:     test    al,al
        jz      wi
        ENDM


DGROUP  group   _DATA,_BSS

_DATA   segment word public use16 'DATA'

EXTRN   C origint9      :DWORD

EXTRN   C parity        :party
EXTRN   C baud          :WORD
EXTRN   C short_loop    :DWORD
EXTRN   C long_loop     :DWORD
EXTRN   C timeslice     :WORD

intmask0        db      ?
intmask1        db      ?

IFDEF   NOISE
speakeron       db      ?
speakeroff      db      ?
ENDIF

_DATA   ends

_BSS    segment word public 'BSS'
_BSS    ends

_TEXT   segment byte public use16 'CODE'
        assume  cs:_TEXT,ds:DGROUP

COMMENT /************************************************
*  Determines CPU type.                                 *
*  Return values:                                       *
*       1 - 8086, 8088, 80186, 80188, NEC V20, NEC V30  *
*       2 - 80286                                       *
*       3 - 80386, 80486                                *
********************************************************/

cpu_type PROC C

        push    sp
        pop     ax
        cmp     ax,sp
        mov     ax,1
        jne     below_286

        mov     ax,7000h
        push    ax
        popf
        pushf
        pop     ax
        test    ax,7000h
        mov     ax,2
        jz      eq_286

        mov     ax,3
eq_286:
below_286:
        ret

cpu_type ENDP


COMMENT /****************************************************
*  Measures time during 2 ^ 8 loops and computes new loop   *
*  values according to the given time constants.            *
*    ( Also copies value of stored original INT 9 vector    *
*  from data segment into code segment )                    *
************************************************************/

make_delay_fact PROC C

        mov     dx,LPT  ; To make it more realistic use same port as getchr
        mov     ecx,100h        ; 2 ^ 8 Loops to evaluate

        ; Load timer 2 with zero value in mode 2
        mov     al,0B4h
        out     43h,al
        xor     ax,ax
        out     42h,al
        xchg    al,ah
        out     42h,al

        cli

        ; Enable timer 2 gate
        in      al,61h
        and     al,0FCh
        out     61h,al
        or      al,1
        out     61h,al

        ; Measure time spent in 256 loops
evaluate:
        in      al,dx
        test    al,10h
        loopd   evaluate

        ; Latch timer 2 tic count
        mov     al,80h
        out     43h,al

        sti

        ; Disable timer 2 gate
        in      al,61h
        and     al,0FCh
        out     61h,al

        ; Read timer 2 tic count
        in      al,42h
        xchg    al,ah
        in      al,42h
        xchg    al,ah
        neg     ax      ; Negate AX because the timer counts down
        dec     ax
        mov     timeslice,ax

        mov     eax,1234DDB3h   ; Value of timer2 input frequency * 2^8
        mov     ecx,SHORTWAIT   ; ( 1.1931817 [MHz] * 256 ÷ 1234DDB3h )
        mul     ecx
        movzx   ecx,timeslice
        div     ecx
        mov     short_loop,eax

        mov     eax,1234DDB3h
        mov     ecx,LONGWAIT
        mul     ecx
        movzx   ecx,timeslice
        div     ecx
        mov     long_loop,eax

        ; Make copy of original INT 9 vector into code segment to simplify
        ; keyboard interrupt routine

        mov     eax,origint9
        mov     cs:cs_origint9,eax

        ret

make_delay_fact ENDP


COMMENT /**************************************************
*    Outputs byte ( 8 bits ), preceded by one start bit   *
*  through pin 17 of LPT1 into Casio fx-7700GB data port  *
*  then terminates the sequence with suitable parity bit  *
*  ( if stated so ) and with two stop bits.               *
*    Routine uses previously selected baud rate.          *
**********************************************************/

putchr PROC C

ARG value :BYTE

        mov     bh,value
        mov     bl,bh
        test    bh,bh
        setpe   bh
        xor     bh,parity
        shl     bh,3

        not     bl
        rol     bl,3

        ;Initialize interrupt sequence

        call    init_timer0 C, baud

        ;Forse output line to zero

        mov     dx,37Ah
        mov     al,8h
        out     dx,al

        IFDEF   NOISE
        mov     al,speakeron
        out     61h,al
        ENDIF

        call    int8on

        mov     cx,8

        ; Send 8 bits of byte
put_next_bit:

        waitint

        IFDEF   NOISE
        bt      bx,3
        jc      @@timer_off
        mov     al,speakeroff
        jmp     @@eo_noise

@@timer_off:
        mov     al,speakeron
@@eo_noise:
        out     61h,al
        ENDIF

        mov     al,bl
        out     dx,al
        ror     bl,1
        loop    put_next_bit

        waitint

        cmp     parity,PARITY_NONE
        je      @@skip_parity

        ;Send parity bit

        mov     al,bh
        out     dx,al

        waitint

@@skip_parity:
        ;Send stoppbit

        xor     al,al
        out     dx,al

        ;Send second stoppbit
        waitint

        ;Wait for end of second stoppbit
        waitint

        call    int8off

        ret

putchr ENDP


COMMENT /***************************************************
*    Routine monitors Casio fx-7700GB output port through  *
*  pin 13 of LPT1 till it encounters start bit or till     *
*  it runs out of given time.                              *
*    In first case the routine reads byte ( 8 bits ) and   *
*  parity bit ( if stated so ) from LPT1, compares parity  *
*  of read byte with parity bit therefore determinating    *
*  validity of read byte:                                  *
*    If it checks OK, it moves the byte to given location  *
*  in memory returning with zero value in AX.              *
*  If not, it just returns with parity error value in AX.  *
*    In second case the routine returns with timeout       *
*  value in AX.                                            *
*    After successful read it waits for the period of two  *
*  stop bits and returns with zero value in AX.            *
*                                                          *
*  Return values:                                          *
*       0          - byte read OK.                         *
*       TIMEOUT    - run out of time                       *
*       PARITY_ERR - parity error encountered              *
*       USER_BREAK - somebody has pressed [ESC] key        *
*                                                          *
*  Routine uses previously selected baud rate.             *
***********************************************************/

getchr PROC C

ARG  char :NEAR PTR BYTE, timo :DWORD
USES di

        mov     ecx,timo
        mov     dx,LPT
        mov     cs:activate,1
        xor     ah,ah

        ; Wait for start bit
marked:
        in      al,dx
        test    al,10h
        loopdnz marked

        mov     cs:activate,0

        jz      read_byte

        test    ah,ah
        jnz     not_timeout

        mov     ax,TIMEOUT
        ret

not_timeout:
        mov     ax,USER_BREAK
        ret

read_byte:
        call    init_timer0 C, baud
        call    int8on

        IFDEF   NOISE
        mov     al,speakeron
        out     61h,al
        ENDIF

        mov     cx,8

        ; Receive 8 data bits
get_next_bit:
        waitint

        in      al,dx
        bt      ax,4

        IFDEF   NOISE
        jc      @@timer_off
        mov     al,speakeron
        jmp     @@eo_noise

@@timer_off:
        mov     al,speakeroff
@@eo_noise:
        out     61h,al
        ENDIF

        rcr     bl,1
        loop    get_next_bit

        mov     di,char
        mov     [di],bl

        cmp     parity,PARITY_NONE
        je      @@skip_parity

        ;Wait for parity bit
        waitint

        in      al,dx
        bt      ax,4
        setnc   bh
        xor     bh,parity

@@skip_parity:
        ;Wait for first stoppbit
        waitint

        ;Wait for second stoppbit
        waitint

        ;Wait for end of second stoppbit
        waitint

        call    int8off

        cmp     parity,PARITY_NONE
        je      no_parity_check

        test    bl,bl
        setpe   bl
        xor     bl,bh
        jnz     parity_error

no_parity_check:
        xor     ax,ax
        ret

parity_error:
        mov     ax,PARITY_ERR
        ret

getchr ENDP


COMMENT /***************************************
*  Initializes timer 0 with divisor according  *
*  to selected baud rate.                      *
***********************************************/

init_timer0 PROC C

ARG count :WORD

        mov     al,34h
        out     43h,al
        mov     ax,count
        out     40h,al
        mov     al,ah
        out     40h,al

        ret

init_timer0 ENDP


COMMENT /*********************************************
*  Initializes certain system variables to speed up  *
*  processing at the time of transmission, and       *
*  set pin 17 of LPT1 to high logical level          *
*  therefore assigning to it marked status.          *
*****************************************************/

highlevel PROC C

        ; Initialize mask variables

        in      al,21h
        and     al,0FEh ;Timer int enabled
        mov     intmask0,al
        or      al,1    ;Timer int disabled
        mov     intmask1,al

        IFDEF   NOISE
        in      al,61h
        and     al,0FCh
        mov     speakeroff,al
        or      al,2
        mov     speakeron,al
        ENDIF

        mov     dx,37Ah
        xor     al,al
        out     dx,al
        ret

highlevel ENDP


COMMENT /******************************************
*      Mask timer interrupt off                   *
**************************************************/

int8on  PROC C

        cli
        mov     al,intmask0
        out     21h,al
        sti
        ret

int8on  ENDP


COMMENT /******************************************
*      Mask timer interrupt to on                 *
**************************************************/

int8off PROC C

        cli
        mov     al,intmask1
        out     21h,al
        sti
        ret

int8off ENDP


COMMENT /*********************************************
*  Timer interrupt service routine to mov FFh into   *
*  AL which is monitored by procedure, waiting for   *
*  interrupt to arrive.                              *
*****************************************************/

myint   PROC

        mov     al,20h  ;Send EOI to first PICU
        out     20h,al

        mov     al,0FFh ;Raise interrupt occurred flag

        iret

myint   ENDP


COMMENT /************************************************
*  Keyboard interrupt service routine to capture [ESC]  *
*  key depressions and to swallow other scan codes if   *
*  global variable <activate> is set to nonzero.        *
********************************************************/

my_keyb_int PROC

        cmp     cs:activate,0
        jne     active

        ; Jump to original interrupt routine

        jmp     cs:dword ptr cs_origint9

active:
        push    ax
        in      al,60h  ; Read the Scan Code
        push    ax

        ; Clear OBF flag

        in      al,61h
        mov     ah,al
        or      al,80h
        out     61h,al
        xchg    ah,al
        out     61h,al

        mov     al,20h
        out     20h,al

        pop     ax
        cmp     al,01   ; Compare with Scan Code for ESC key
        pop     ax
        jne     not_esc

        mov     ah,0FFh ; Flag, that ESC key was pressed
        mov     ecx,1   ; and terminate loop by nearly clearing ECX register

not_esc:
        iret

cs_origint9     dd      ?       ; Store for original keyboard interrupt vector
activate        db      0

my_keyb_int ENDP


        ;Data

        public  C intmask0
        public  C intmask1

        IFDEF   NOISE
        public  C speakeron
        public  C speakeroff
        ENDIF

        ;Functions

        public  C myint
        public  C my_keyb_int
        public  C init_timer0
        public  C highlevel
        public  C int8on
        public  C int8off
        public  C putchr
        public  C getchr
        public  C cpu_type
        public  C make_delay_fact

_TEXT   ends

        end
