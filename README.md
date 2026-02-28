# casiocalc-comm-msdos-lpt
MS-DOS menu-driven utility for communicating with Casio calculator via PC LPT

This utility is written by me in year 1993,
the same year the Casio fx-7700GB was released.


## INTRODUCTION

Some Casio calculators, including fx-7700GB, have 2,5mm plug for sending and
receiving data with other calculators or PCs.

 - This line carries RS-232 TTL 5V signal;
 - Can be configured to use 1200, 2400, 4800 and 9600 baud data rate;
 - Can be set to Even, Odd or None parity;

Casio provided optional cables for connecting two calculators, part numbers
SB-60 or SB-62, and also optional battery powered level-shifter kits
for connecting the calculator to PC via native serial interface, part numbers
FA-100 or FA-121 and maybe also others.


## REQUIREMENTS

This utility was written to circumvent the need for level shifter and to
communicate with PC via single cable. The idea was to use PC LPT port,
because the voltage levels of PC parallel port are compatible with the
calculator communication voltage levels.

Cable specification:

| fx-7700GB | DB25 LPT         |
|-----------|------------------|
| Tip       | Pin 13 (SLCT)    |
| Ring      | Pin 17 (SLCT IN) |
| Sleeve    | Pin 25 (GND)     |


## PLATFORM AND DEVELOPMENT ENVIRONMENT

The utility is written for MS-DOS. It takes control of whole system,
modifies interrupts, uses system timer chip and manages directly LPT hardware.
It is meant to be used on the single-threaded system.

The binaries are built, using Borland development tools:
Borland C++ v3.1
Borland Turbo Assembler v3.1
Borland H2ASH Processor v3.1
Borland Make v3.6
Borland Turbo Link v5.1


## BUILDING

If the Borland development environment is in place, then in project directory
type `make casio.exe` or `make prg2txt.exe`.


## BINARIES

Two binaries are included:

CASIO.EXE - menu driven simplistic interface, just run it and follow the UI.
	It works in DosBox, but unfortunately without the communication function.
	For this, the utility has to be run on bare metal, that has the standard
	chipset, at least with the i8259 PICU, i8253 timer chip and i8255 PPI.

PRG2TXT.EXE - command line utility for converting binary program files into human
	readable text files.
	Usage:
	PRG2TXT *programfile* *textfile*


## CONCLUSION

This is for retro enthusiasts who think highly of Casio retro calculators,
especially the fx-7700GB. Who have old MS-DOS PC with parallel port at hand,
or maybe new "Retro PC" like Pocket 386 from AliExpress. And who are eager
to use PC for backupping or storing the Calculator programs.



