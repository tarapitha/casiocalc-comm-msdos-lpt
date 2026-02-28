#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <io.h>
#include <dos.h>
#include <errno.h>
#include <dir.h>
#include "casio.h"

#define ROWS            7
#define FIRSTROW        17
#define COLUMNS         5
#define FIRSTCOLUMN     5

#define MAXMENU 7
#define MENUITEMLEN 20

typedef struct menudef
 {int   x, y, pos;
  char  messages[ MAXMENU ][ MENUITEMLEN ];
 };

extern void make_delay_fact( void );
extern int  cpu_type( void );
extern int  getchr( unsigned char *, unsigned long );
extern void putchr( unsigned char );
extern void highlevel( void );
extern void int8off( void );
extern void int8on( void );
extern void init_timer0( unsigned );

extern void interrupt myint( void );
extern void interrupt my_keyb_int( void );

int  transmit( void );
int  receive( void );
void trerror( int );
void statusline( void );
int  readstring( int x, int y, int len, char * );
int  findfile( char * );
int  menu( struct menudef * );
void setsystime( void );
void saveconfig( void );
void check_386( void );
int c_break( void );
void setmyint( void );
void origint( void );
void far int24_handler( unsigned, unsigned, unsigned far * );

#pragma startup highlevel 64
#pragma startup int8off   65
#pragma startup setmyint  66
#pragma startup check_386 67
#pragma startup make_delay_fact 68

#pragma exit origint 67
#pragma exit int8on  66
#pragma exit setsystime 65
#pragma exit saveconfig 64

void interrupt ( *origint8 )(); // Store for original timer interrupt
void interrupt ( *origint9 )(); // Store for original keyboard interrupt

unsigned baud = b9600;          // Default baud rate
party parity = PARITY_EVEN;     // Default parity check

FILE *p, *config;
char fname[80], fshab[80] = "";
struct ffblk ffblk;
unsigned char byte;
unsigned char sum, mode = 0, type;
unsigned char mass[50*1024u];
int st, exist, all = 0;
int i, j, c, status;
char errdrive;
union
 {unsigned word;
  unsigned char byte[2];
 }count;

unsigned long short_loop, long_loop;
unsigned timeslice;

struct menudef firstmenu = { 18, 14, 2,
               "Transmit", "Receive", "File", "Baud", "Parity" };
struct menudef baudmenu = { 26, 14, 3, "1200", "2400", "4800", "9600" };

struct menudef paritymenu = { 28, 14, 0, "EVEN", "ODD", "NONE" };

struct menudef endmenu = { 36, 14, 0, "Yes", "No" };

/*******************************************************
Function: main

Description:
*******************************************************/

int main(int argc, char *argv[] )
{int i, ch, end = 0;

 _harderr( int24_handler );

 if( argc == 2 )
   strcpy( fname, argv[ 1 ] );

 if( ( config = fopen( "casio.cnf", "rb" ) ) != NULL )
  {ch = getc( config );
   baudmenu.pos = ch;
   switch( ch )
    {case 0:
       baud = b1200;
       break;
     case 1:
       baud = b2400;
       break;
     case 2:
       baud = b4800;
       break;
     case 3:
       baud = b9600;
       break;
    }
   ch = getc( config );
   paritymenu.pos = ch;
   switch( ch )
    {case 0:
       parity = PARITY_EVEN;
       break;
     case 1:
       parity = PARITY_ODD;
       break;
     case 2:
       parity = PARITY_NONE;
       break;
    }
   fclose( config );
  }

 statusline();
 while( !end )
  {
   gotoxy( 30, 9 );
   clreol();
   cputs( "Parity: " );
   switch( parity )
    {case PARITY_EVEN:
       cputs( "EVEN" );
       break;
     case PARITY_ODD:
       cputs( "ODD" );
       break;
     case PARITY_NONE:
       cputs( "NONE" );
       break;
    }
   gotoxy( 30, 10 );
   clreol();
   cputs( "Baud  : " );
   switch( baud )
    {case b1200:
       cputs( "1200" );
       break;
     case b2400:
       cputs( "2400" );
       break;
     case b4800:
       cputs( "4800" );
       break;
     case b9600:
       cputs( "9600" );
    }
   cputs( " bps" );
   gotoxy( 30, 11 );
   clreol();
   printf( "File  : %s", fname );
   i = menu( &firstmenu );
   gotoxy( 20, 16 );
   clreol();
   switch( i )
    {case -1:
       statusline();
       gotoxy( 20, 11 );
       cputs(" Do you really want to end this program ?" );
       i = menu( &endmenu );
       if( !i )
         end = 1;
       statusline();
       break;
     case 0:
       if( strlen( fname ) != 0 )
        {if( transmit() < 0 )
          {cputs( "Press a key ..." );
           getch();
          }
         statusline();
        }
       else
        {gotoxy( 20, 16 );
         cputs( "You must supply file name to work with ..." );
         firstmenu.pos = 2;
        }
       break;
     case 1:
       if( strlen( fname ) != 0 )
        {if( receive() < 0 )
          {cputs( "Press a key ..." );
           getch();
          }
         statusline();
        }
       else
        {gotoxy( 20, 16 );
         cputs( "You must supply file name to work with ..." );
         firstmenu.pos = 2;
        }
       break;
     case 2:
       gotoxy( 18, 14 );
       clreol();
       cputs("Filename: ");
       if( !readstring( 18 + strlen( "Filename: " ), 14, 50, fshab ) ||
                                                                fshab[0] == 0 )
        {if( fshab[0] != 0 )
          {strcpy( fname, fshab );
           if( !findfile( fname ) )
             strcpy( fshab, fname );
          }
         else
          {strcpy( fname, "*.prg" );
           if( !findfile( fname ) )
             strcpy( fshab, fname );
          }
        }
       statusline();
       break;
     case 3:
       i = menu( &baudmenu );
       switch( i )
        {case -1:
           break;
         case 0:
           baud = b1200;
           break;
         case 1:
           baud = b2400;
           break;
         case 2:
           baud = b4800;
           break;
         case 3:
           baud = b9600;
           break;
        }
       break;
     case 4:
       i = menu( &paritymenu );
       switch( i )
        {case -1:
           break;
         case 0:
           parity = PARITY_EVEN;
           break;
         case 1:
           parity = PARITY_ODD;
           break;
         case 2:
           parity = PARITY_NONE;
           break;
        }
    }
  }

 gotoxy( 1, 25 );
 clreol();
 return 0;
}

/*******************************************************
Function: transmit

Description:
*******************************************************/

int transmit(void)
{
 clrscr();
 if(( p = fopen(fname,"rb")) == NULL )
  {perror(fname);
   return -1;
  }

 type = getc(p);
 if( type != ONE && type != ALL )
  {mode = type;
   type = ONE;
  }
 else
   if( type == ONE )
     mode = getc(p);

 i = 0;
 while(( c = getc(p)) != EOF && ( unsigned ) i < sizeof(mass) )
   mass[i++] = (unsigned char) c;

 if( type == ALL )
   count.word = i - 38*5 + 2;
 else
   count.word = i + 3;

 fclose(p);

 // Start transmitting

 puts( "\n==TRANSMITTING==\n" );
 puts( "\nPress [ESC] to interrupt\n" );

 putchr(SYN);

 st = getchr(&byte, short_loop);
 if( st )
  {trerror( st );
   return -1;
  }

 if( byte != DC3 )
  {printf("1) Unexpected symbol -> %02X\n", byte);
   putchr( RER );
   return -1;
  }

 sum = HDR + type + count.byte[0] + count.byte[1] + mode + 31 * EOP;

 putchr(DIC);
 putchr(HDR);
 putchr(type);
 putchr(0);
 putchr(count.byte[1]);
 putchr(count.byte[0]);
 putchr(mode);
 putchr(0);
 for(i = 0; i < 31; i++)
   putchr(EOP);

 putchr(-sum);

 st = getchr(&byte, long_loop);
 if( st )
  {trerror( st );
   return -1;
  }

 switch( byte )
  {case ACK:
     break;
   case AU:
     printf("Program area already used. Overwrite? (Y/N) ");
     while( toupper(c = getch()) != 'Y' && toupper(c) != 'N')
      {sound(440);
       delay(10);
       nosound();
      }
     printf("%c\n", toupper(c));
     if( toupper(c) == 'N' )
      {putchr(NAC);
       return 0;
      }
     else
      {putchr(ACK);
       st = getchr(&byte, long_loop);
       if( st )
        {trerror( st );
         return -1;
        }

       if( byte != ACK )
        {printf("3) Unexpected symbol -> %02X\n",byte);
         putchr( RER );
         return -1;
        }
      }
     break;
   case CER:
     puts("** Receiver error: Header checksum invalid");
     return -1;
   case RER:
     puts("** Receiver error: Invalid header");
     return -1;
   case IMO:
     puts("** Receiver error: Incorrect mode");
     return -1;
   case MF:
     puts("** Receiver error: Memory full!");
     return -1;
   default:
     printf("2) Unexpected symbol -> %02X\n", byte);
     putchr( RER );
     return -1;
  }

 if( type == ONE )
  {putchr(DIC);
   sum = EOP;

   for(i = 0; i < count.word - 3; i++)
    {sum += mass[i];
     putchr(mass[i]);
    }

   putchr(EOP);
   putchr(-sum);
  }
 else
  {putchr(DIC);
   sum = 0;

   for(i = 0; i < 38*5; i++)
    {sum += mass[i];
     putchr(mass[i]);
    }

   putchr(-sum);

   st = getchr(&byte, long_loop);
   if( st )
    {trerror( st );
     return -1;
    }

   switch( byte )
    {case ACK:
       break;
     case CER:
       puts("** Receiver error: Directory checksum invalid");
       return -1;
     case RER:
       puts("** Receiver error: Invalid header");
       return -1;
     default:
       printf("2) Unexpected symbol -> %02X\n", byte);
       putchr( RER );
       return -1;
    }

   putchr(DIC);
   sum = 0;

   for(i = 38*5; i < count.word + 38*5 - 2; i++)
    {sum += mass[i];
     putchr(mass[i]);
    }

   putchr(-sum);
  }

 st = getchr(&byte, long_loop);
 if( st )
  {trerror( st );
   return -1;
  }

 if( byte == ACK )
   printf("Ok\n");
 else
   if( byte == CER )
    {puts("** Receiver error: Data checksum invalid");
     return -1;
    }
   else
    {printf("4) Unexpected symbol -> %02X\n",byte);
     return -1;
    }

 return 0;
}

/*******************************************************
Function: receive

Description:
*******************************************************/

int receive( void )
{
 clrscr();
 exist = !findfirst(fname, &ffblk, 0);

 puts( "\n==RECEIVING==\n" );
 puts( "\nPress [ESC] to interrupt\n" );

 st = getchr(&byte, long_loop);
 if( st )
  {trerror( st );
   return -1;
  }

 if( byte != SYN )
  {printf("1) Unexpected symbol -> %02X\n", byte);
   putchr( RER );
   return -1;
  }

 putchr(DC3);

 st = getchr(&byte, long_loop);

 if( st )
  {trerror( st );
   return -1;
  }

 if( byte != DIC )
  {printf("2) Unexpected symbol -> %02X\n", byte);
   sleep( 2 );
   putchr( RER );
   return -1;
  }
 for( i = 0; i < 39; i++)
   if( ( st = getchr(&mass[i], long_loop ) ) != 0 )
     break;

 if( st )
  {trerror( st );
   return -1;
  }

 for( i = sum = 0; i < 39; i++)
   sum += mass[i];

 if( sum )
  {puts( "Header checksum invalid!" );
   putchr( CER );
   return -1;
  }

 mode = mass[5];
 count.byte[1] = mass[3];
 count.byte[0] = mass[4];
 if( count.word > sizeof( mass ) )
  {puts("Not enough memory to store all the data");
   putchr( MF );
   return -1;
  }

 if( exist )
  {putchr(AU);
   st = getchr(&byte, long_loop);
   if( st )
    {trerror( st );
     return -1;
    }

   switch( byte )
    {case ACK:
       putchr(ACK);
       break;
     case NAC:
       return 0;
     default:
       printf("3) Unexpected symbol -> %02X\n", byte);
       putchr( RER );
       return -1;
    }
  }
 else
   putchr(ACK);

 if( ( type = mass[1] ) == ONE )
  {st = getchr(&byte, long_loop);
   if( st )
    {trerror( st );
     return -1;
    }

   if( byte != DIC )
    {printf("4) Unexpexted symbol -> %02X\n", byte);
     sleep( 2 );
     putchr( RER );
     return -1;
    }

   for(i = 0; i < count.word - 1; i++)
     if( ( st = getchr(&mass[i], long_loop ) ) != 0 )
       break;

   if( st )
    {trerror( st );
     return -1;
    }

   for( i = sum = 0; i < count.word - 1; i++)
     sum += mass[i];

   if( sum )
    {puts( "Program checksum invalid!" );
     putchr( CER );
     return -1;
    }

   //Store data

   while( ( p = fopen( fname, "wb" ) ) == NULL )
    {if( status == 0 )
      {printf( "\nPlease make disk %c: writeable and\n", errdrive );
       puts( "  Press [ENTER] to store received data or" );
       puts( "  Press [ESC] to cancel without save" );
       while( ( c = getch() ) != 27 && c != 13 );
       if( c != 27 )
         continue;
      }
     if( status == 2 )
      {printf( "\nPlease insert disk into drive %c: and\n", errdrive );
       puts( "  Press [ENTER] to store received data or" );
       puts( "  Press [ESC] to exit without save" );
       while( ( c = getch() ) != 27 && c != 13 );
       if( c != 27 )
         continue;
      }
     perror(fname);
     putchr( RER );
     return -1;
    }

   putc(type, p);
   putc(mode, p);
   for(i = 0; i < count.word - 3; i++)
     putc(mass[i],p);

   fclose(p);

   putchr(ACK);
  }
 else
   if( mass[1] == ALL )
    {st = getchr(&byte, long_loop);
     if( st )
      {trerror( st );
       return -1;
      }

     if( byte != DIC )
      {printf("5) Unexpected symbol -> %02X\n", byte);
       return -1;
      }                               // 1 - control code
     for(i = 0; i < 38*5+1; i++)      //38 - programs count  5 - bpp
       if( ( st = getchr(&mass[i], long_loop ) ) != 0 )
         break;

     if( st )
      {trerror( st );
       return -1;
      }

     for( i = sum = 0; i < 38*5+1; i++)
       sum += mass[i];

     if( sum )
      {puts( "Directory checksum invalid!" );
       putchr( CER );
       return -1;
      }

     while( ( p = fopen( fname, "wb" ) ) == NULL )
      {if( status == 0 )
        {printf( "\nPlease make disk %c: writeable and\n", errdrive );
         puts( "  Press [ENTER] to store received data or" );
         puts( "  Press [ESC] to cancel without save" );
         while( ( c = getch() ) != 27 && c != 13 );
         if( c != 27 )
           continue;
        }
       if( status == 2 )
        {printf( "\nPlease insert disk into drive %c: and\n", errdrive );
         puts( "  Press [ENTER] to store received data or" );
         puts( "  Press [ESC] to exit without save" );
         while( ( c = getch() ) != 27 && c != 13 );
         if( c != 27 )
           continue;
        }
       perror(fname);
       putchr( RER );
       return -1;
      }

     putc( type, p );
     for(i = 0; i < 38*5; i++)
       putc(mass[i],p);

     fclose( p );

     putchr(ACK);

     st = getchr(&byte, long_loop);

     if( st )
      {trerror( st );
       return -1;
      }

     if( byte != DIC )
      {printf("6) Unexpexted symbol -> %02X\n", byte);
       return -1;
      }
     for(i = 0; i < count.word - 1; i++)
       if( ( st = getchr(&mass[i], long_loop ) ) != 0 )
         break;

     if( st )
      {trerror( st );
       return -1;
      }

     for( i = sum = 0; i < count.word - 1; i++)
       sum += mass[i];

     if( sum )
      {puts( "All programs: checksum invalid!" );
       putchr( CER );
       return -1;
      }

     if(( p = fopen(fname,"ab")) == NULL )
      {perror(fname);
       putchr( RER );
       return -1;
      }

     for(i = 0; i < count.word - 2; i++)
       putc(mass[i],p);

     fclose(p);

     putchr(ACK);
    }
   else
    {printf("Unexpected attribute of header -> %02X\n", byte);
     sleep( 2 );
     putchr( RER );
     return -1;
    }

 return 0;
}

/*******************************************************
Function: menu

Description:
*******************************************************/

int menu( struct menudef *mm )
{int i, j, max = 0, temp, end = 0;

 _setcursortype( _NOCURSOR );
 for( i = 0; i < MAXMENU; i++ )
  {if( ( temp = strlen( mm->messages[ i ] ) ) > max )
    max = temp;
  }
 max += 2;

 textattr( 0x07 );
 for( i = 0; i < MAXMENU; i++ )
  {if( strlen( mm->messages[ i ] ) != 0 )
    {gotoxy( mm->x + i*max, mm->y );
     cputs( mm->messages[ i ] );
    }
  }
 textattr( 0x70 );
 gotoxy( mm->x + mm->pos*max, mm->y );
 cputs( mm->messages[ mm->pos ] );
 textattr( 0x07 );

 i = j = mm->pos;
 while( !end )
  {switch( getch() )
    {case 0:
       switch( getch() )
        {case 75:
           if( --i < 0 )
             i = MAXMENU - 1;
           while( strlen( mm->messages[ i ] ) == 0 )
             i--;
           break;
         case 77:
           i++;
           while( strlen( mm->messages[ i ] ) == 0 && i < MAXMENU )
             i++;
           if( i >= MAXMENU )
             i = 0;
           break;
         case 72:
           i++;
           while( strlen( mm->messages[ i ] ) == 0 && i < MAXMENU )
             i++;
           if( i >= MAXMENU )
             i = 0;
           break;
         case 80:
           if( --i < 0 )
             i = MAXMENU - 1;
           while( strlen( mm->messages[ i ] ) == 0 )
             i--;
        }
       break;
     case 13:
       end = 1;
       break;
     case 27:
       gotoxy( mm->x, mm->y );
       clreol();
       _setcursortype( _NORMALCURSOR );
       return -1;
    }
   if( i != j )
    {gotoxy( mm->x + i*max, mm->y );
     textattr( 0x70 );
     cputs( mm->messages[ i ] );
     gotoxy( mm->x + j*max, mm->y );
     textattr( 0x07 );
     cputs( mm->messages[ j ] );
     j = i;
    }
  }
 gotoxy( mm->x, mm->y );
 clreol();
 _setcursortype( _NORMALCURSOR );
 mm->pos = i;

 return i;
}

/*******************************************************
Function: readstring

Description:
*******************************************************/

int readstring( int x, int y, int len, char filename[] )
{int c, i, j, k;
 char storename[80];

 strcpy( storename, filename );
 gotoxy( x, y );
 cputs(filename);
 while(!kbhit());
 if( ( c = getch() ) == 0 || c == 8 || c == 13 )
   i = strlen( filename );
 else
  {filename[0]='\0';
   i=0;
  }
 ungetch( c );
 gotoxy( x, y );
 for( j = 0; j < len; j++ )
   putch( ' ' );
 gotoxy( x, y );
 cputs( filename );
 while( ( c = getch() ) != 13 && c != 27 )
  {if( isalpha( c ) || isdigit( c ) ||
       ( strchr( "!@#$%&*()_-{}.*\\?:~`'", c ) && c != 0 ) )
    {if( strlen( filename ) < len )
      {putch( c );
       k = strlen( filename );
       for( j = k; j > i; j-- )
        {gotoxy( j + x, y );
         putch( filename[ j ] = filename[ j - 1 ] );
        }
       filename[k+1]='\0';
       filename[i++]=c;
      }
     else
       putchar( 7 );
    }
   else
    {if(c == 8)
      {if(i>0)
        {i--;
         gotoxy(i+x,y);
         k=strlen(filename);
         for(j=i;j<k;j++)
          putch(filename[j]=filename[j+1]);
        }
      }
     else
      if(c == 0)
       {switch( c = getch() )
         {case 'H':/*Up*/
          case 'K':/*Left*/
            if( i > 0 )
              i--;
            break;
          case 'P':/*Down*/
          case 'M':/*Right*/
            if(i < len && filename[ i ] != '\0' )
              i++;
            break;
          case 'S':/*Del*/
            gotoxy( i + x, y );
            k=strlen( filename );
            for( j = i; j < k; j++ )
              putch( filename[ j ] = filename[ j + 1 ] );
         }
       }
    }
   gotoxy( i + x, y );
  }

 if( c == 27 )
  {strcpy( filename, storename );
   return -1;
  }

 return 0;
}

/*******************************************************
Function: findfile

Description:
*******************************************************/

int findfile( char shablon[] )
{int i, j, k, l, m, c;
 int end, nihe, done;
 char *cp;
 char dir[80], drive[3], name[9], ext[5];
 char list[512][13];
 char shablon2[80];
 int p;
 struct ffblk ffblk;

 i = 0;
 _splitpath( shablon, drive, dir, name, ext );
 if( !( name[ 0 ] | ext[ 0 ] ) )
   _makepath( shablon, drive, dir, "*", ".*" );
 done = findfirst( shablon, &ffblk, 0);
 while( !done && i < 512 )
  {strcpy( list[i], "        ");
   if( ( cp = strchr( ffblk.ff_name, '.' ) ) != 0 )
    {strcpy( &list[i][9], cp + 1 );
     *cp = 0;
     strcpy( list[i], ffblk.ff_name );
     list[ i ][ strlen( ffblk.ff_name ) ] = ' ';
     list[ i ][ 8 ] = '.';
     for( j = 9; j < 12; j++ )
       if( list[ i ][ j ] == 0 )
        {for( k = j; k < 12; k++ )
           list[ i ][ k ] = ' ';
         list[ i ][ 12 ] = 0;
        }
    }
   else
    {strcpy( list[ i ], ffblk.ff_name );
     for( j = strlen( list[ i ] ); j < 12; j++ )
       list[ i ][ j ] = ' ';
     list[ i ][ j ] = 0;
    }
   i++;
   done = findnext( &ffblk );
  }

 if( i )
  {c = 13;
   nihe = j = k = 0;
   if( i != 1 )
    {textattr( 0x07 );
     for( k = 0; k < COLUMNS && ROWS * k < i; k++ )
       for( j = 0; j < ROWS && j + ROWS * k < i; j++ )
        {gotoxy( FIRSTCOLUMN + 15 * k, FIRSTROW + j );
         cputs( list[ ROWS * k + j ] );
        }
     gotoxy( FIRSTCOLUMN, FIRSTROW );
     textattr( 0x70 );
     cputs( list[ 0 ] );
     textattr( 0x07 );

     _setcursortype( _NOCURSOR );
     k = j = nihe = 0;
     while( ( c = getch() ) != 13 && c != 27 )
      {gotoxy( FIRSTCOLUMN + 15 * k, FIRSTROW + j );
       textattr( 0x07 );
       cputs( list[ ROWS * ( k + nihe ) + j ] );
       if(c == 0)
        {switch( c = getch() )
          {case 'H':        // Up
             if( j - 1 >= 0 )
               j--;
             break;
           case 'K':        // Left
             if( k - 1 >= 0 )
               k--;
             else
               if( nihe )
                {nihe--;
                 textattr( 0x07 );
                 for( l = 0; l < ROWS; l++ )
                  {gotoxy( FIRSTCOLUMN, FIRSTROW + l );
                   clreol();
                  }
                 for( m = 0; m < COLUMNS && ROWS * ( m + nihe ) < i; m++ )
                   for( l = 0; l < ROWS && l + ROWS * ( m + nihe ) < i; l++ )
                    {gotoxy( FIRSTCOLUMN + 15 * m, FIRSTROW + l );
                     cputs( list[ ROWS * ( m + nihe ) + l ] );
                    }
                 gotoxy( FIRSTCOLUMN + 15 * k, FIRSTROW + j );
                 textattr( 0x70 );
                 cputs( list[ ROWS * ( k + nihe ) + j ] );
                 textattr( 0x07 );
                }
             break;
           case 'P':        // Down
             if( j + 1 < ROWS && ROWS * ( k + nihe ) + j + 1 < i )
               j++;
             break;
           case 'M':        // Right
             if( k + 1 < COLUMNS && ROWS * ( k + nihe + 1 ) + j < i )
               k++;
             else
               if( ROWS * ( k + nihe + 1 ) + j < i  )
                {nihe++;
                 textattr( 0x07 );
                 for( l = 0; l < ROWS; l++ )
                  {gotoxy( FIRSTCOLUMN, FIRSTROW + l );
                   clreol();
                  }
                 for( m = 0; m < COLUMNS && ROWS * ( m + nihe ) < i; m++ )
                   for( l = 0; l < ROWS && l + ROWS * ( m + nihe ) < i; l++ )
                    {gotoxy( FIRSTCOLUMN + 15 * m, FIRSTROW + l );
                     cputs( list[ ROWS * ( m + nihe ) + l ] );
                    }
                 gotoxy( FIRSTCOLUMN + 15 * k, FIRSTROW + j );
                 textattr( 0x70 );
                 cputs( list[ ROWS * ( k + nihe ) + j ] );
                 textattr( 0x07 );
                }
          }
        }
       gotoxy( FIRSTCOLUMN + 15 * k, FIRSTROW + j );
       textattr( 0x70 );
       cputs( list[ ROWS * ( k + nihe ) + j ] );
       textattr( 0x07 );
      }
     _setcursortype( _NORMALCURSOR );
    }
   if( c == 13 )
    {for( i = 0; i < 8 &&
                 list[ ROWS * ( k + nihe ) + j ][ i ] != ' ' &&
                 list[ ROWS * ( k + nihe ) + j ][ i ] != '.'; i++ );
     if( list[ ROWS * ( k + nihe ) + j ][ i ] == '.' )
       strcpy( shablon, list[ ROWS * ( k + nihe ) + j ] );
     else
      {list[ ROWS * ( k + nihe ) + j ][ i ] = 0;
       strcpy( shablon, list[ ROWS * ( k + nihe ) + j ] );
       strcat( shablon, &list[ ROWS * ( k + nihe ) + j ][ 8 ] );
      }
     for( i = 0; i < 12; i++ )
       if( shablon[ i ] == ' ' )
        {shablon[ i ] = 0 ;
         break;
        }
     _splitpath( shablon, NULL, NULL, name, ext );
     _makepath( shablon, drive, dir, name, ext );
     strupr( shablon );
    }
   else
    {if( !findfirst( shablon, &ffblk, 0) )
      {if( strcmp( shablon, ffblk.ff_name ) )
        {shablon[ 0 ] = 0;
         return -1;
        }
      }
     else
      {if( ( p = _creat( shablon, FA_ARCH ) ) < 0 )
        {shablon[ 0 ] = 0;
         return -1;
        }
       close( p );
       unlink( shablon );
      }
    }
  }
 else
  {_splitpath( shablon, drive, dir, name, ext );
   _makepath( shablon2, drive, dir, name, ext );
   if( strcmp( shablon, shablon2 ) )
    {shablon[ 0 ] = 0;
     return -1;
    }
   if( ( p = _creat( shablon, FA_ARCH ) ) < 0 )
    {shablon[ 0 ] = 0;
     return -1;
    }
   close( p );
   unlink( shablon );
  }

 return 0;
}

/*******************************************************
Function: statusline

Description:
*******************************************************/

void statusline( void )
{
 clrscr();
 gotoxy( 28, 4 );
 textattr( 0x07 );
 cputs( "Casio fx-7700GB" );
 gotoxy( 23, 5 );
 cputs( "data communication program" );
 gotoxy( 1, 25 );
 textattr( 0x70 );
 cputs( "  [ESC] to exit  ³     \x1A  to move  ³  [ENTER] to choose ...                 " );
 textattr( 0x07 );
}

/*******************************************************
Function: trerror

Description:
*******************************************************/

void trerror( int st )
{
 switch( st )
  {case PARITY_ERR:
     puts( "Parity error" );
     break;
   case TIMEOUT:
     puts( "Timeout" );
     break;
   case USER_BREAK:
     puts( "Data transfer interrupted by user" );
  }
}

/*******************************************************
Function: setsystime

Description:
*******************************************************/

void setsystime( void )
{time_t t;
 struct tm tm;
 union REGS regs;

 regs.h.ah = 4;
 int86( 0x1A, &regs, &regs );
 tm.tm_mon = 10*((( int )regs.h.dh >> 4) & 0xF ) + (( int )regs.h.dh & 0xF)-1;
 tm.tm_year = 10*((( int )regs.h.cl >> 4) & 0xF ) + (( int )regs.h.cl & 0xF );
 tm.tm_mday = 10*((( int )regs.h.dl >> 4) & 0xF ) + (( int )regs.h.dl & 0xF );

 regs.h.ah = 2;
 int86( 0x1A, &regs, &regs );
 tm.tm_sec = 10*((( int )regs.h.dh >> 4) & 0xF ) + ( ( int )regs.h.dh & 0xF );
 tm.tm_min = 10*((( int )regs.h.cl >> 4) & 0xF ) + ( ( int )regs.h.cl & 0xF );
 tm.tm_hour = 10*((( int )regs.h.ch >> 4) & 0xF ) + ( ( int )regs.h.ch & 0xF );

 t = mktime( &tm );
 stime( &t );
}

/*******************************************************
Function: check_386

Description:
*******************************************************/

void check_386( void )
{
 switch( cpu_type() )
  {case 1:
     puts( "\nSorry, but this program is making use of 80386 specific instructions" );
     puts( "       and therefore can't run on PC/XT type computers ..." );
     exit( 1 );
   case 2:
     puts( "\nSorry, but this program is making use of 80386 specific instructions" );
     puts( "       and therefore can't run on computers based on 286 CPU ..." );
     exit( 1 );
   case 3:
     puts( "\n80386 compatible CPU detected" );
     delay( 2000 );
  }
}


int c_break( void )
{
 exit( 0 );
 return 1;
}


void setmyint( void )
{
 origint8 = getvect( TIMER_INT );
 setvect( TIMER_INT, myint );
 origint9 = getvect( KEYBOARD_INT );
 setvect( KEYBOARD_INT, my_keyb_int );
 ctrlbrk( c_break );
}


void origint( void )
{
 setvect( TIMER_INT, origint8 );
 setvect( KEYBOARD_INT, origint9 );
 init_timer0( 0 );
}


#pragma warn -par

void far int24_handler( unsigned deverr, unsigned errval, unsigned far *devhdr )
{
 if( deverr & 0x8000 )
  {status = -1;
   _hardretn( 5 );
  }

 errdrive = ( deverr & 0xFF ) + 'A';
 status = errval & 0xFF;
 _hardresume( _HARDERR_FAIL );
}

#pragma warn +par


/*******************************************************
Function: saveconfig

Description:
*******************************************************/

void saveconfig( void )
{int ch;

 while( ( config = fopen( "casio.cnf", "wb" ) ) == NULL )
  {if( status == 0 )
    {printf( "\nPlease make disk %c: writeable and\n", errdrive );
     puts( "  Press [ENTER] to save current configuration or" );
     puts( "  Press [ESC] to exit without saving configuration" );
     while( ( c = getch() ) != 27 && c != 13 );
     if( c == 27 )
       return;
     else
       continue;
    }
   if( status == 2 )
    {printf( "\nPlease insert disk into drive %c: and\n", errdrive );
     puts( "  Press [ENTER] to save current configuration or" );
     puts( "  Press [ESC] to exit without saving configuration" );
     while( ( c = getch() ) != 27 && c != 13 );
     if( c == 27 )
       return;
     else
       continue;
    }
   perror( "casio.cnf" );
   return;
  }
 putc( baudmenu.pos, config );
 putc( paritymenu.pos, config );
 fclose( config );
}
