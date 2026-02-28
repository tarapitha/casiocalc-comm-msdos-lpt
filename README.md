# casiocalc-comm-msdos-lpt
MS-DOS menu-driven utility for communicating with Casio calculator via PC LPT

	This utility is written by me in year 1993,
 the same year the Casio fx-7700GB was released.


 - I N T R O D U C T I O N -

Some Casio calculators, including fx-7700GB, have 2,5mm plug for sending and
receiving data with other calculators or PCs.

 - This line carries RS-232 TTL 5V signal;
 - Can be configured to use 1200, 2400, 4800 and 9600 baud data rate;
 - Can be set to Even, Odd or None parity;

Casio provided optional cables for connecting two calculators, part numbers
SB-60 or SB-62, and also optional battery powered level-shifter kits
for connecting the calculator to PC via native serial interface, part numbers
FA-100 or FA-121 and maybe also others.


 - R E Q U R E M E N T S -

This utility was written to circumvent the need for level shifter and to
communicate with PC via single cable. The idea was to use PC LPT port,
because the voltage levels of PC parallel port are compatible with the
calculator communication voltage levels.

Cable specification:

fx-7700GB	DB25 LPT
--------------------------------
Tip		pin 13 (SLCT)
Ring		pin 17 (SLCT IN)
Sleeve		pin 25 (GND)


 - P L A T F O R M   A N D   D E V E L O P M E N T  E N V I R O N M E N T -

The utility is written for MS-DOS. It takes control of whole system,
modifies interrupts, uses system timer chip and manages directly LPT hardware.
It is meant to be used on the single-threaded system.

The binaries are built, using Borland development tools:
Borland C++ v3.1
Borland Turbo Assembler v3.1
Borland H2ASH Processor v3.1
Borland Make v3.6
Borland Turbo Link v5.1


 - B I N A R I E S -

There is two binaries included:

CASIO.EXE - menu driven simplistic interface, just run it and follow the UI.
	It works in DosBox, but unfortunately not the communication function.
	For this, the utility has to be run on bare metal, that has the standard
	chipset, at least with the i8259 PICU, i8253 timer chip and i8255 PPI.

PRG2TXT.EXE - command line utility for converting binary program files into human
	readable text files.
	Usage:
	PRG2TXT <program file> <text file>


 - C O N C L U S I O N -

This is for retro enthusiasts, need Casio fx-7700GB and also old MS-DOS PC
with parallel port, or maybe new "Retro PC" like Pocket 386 from AliExpress.

