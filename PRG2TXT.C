// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Robert Tiismus

/*
* PRG2TXT.C
*
* Utility for converting downloaded Casio fx-7700GB binary iprogram files
* into human-readable text files
*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include "trtable.h"

int main( int argc, char *argv[] )
{FILE *in, *out;
 char outfname[ 80 ], outfn[ 20 ], outfd[ 10 ], outfdir[ 70 ], outfext[ 10 ];
 int i, c, d = 0xF0;

 if( argc != 2 )
  {printf( "Anna k„sureale faili nimi\n" );
   exit( 1 );
  }

 if(( in = fopen( argv[ 1 ], "rb" ) ) == NULL )
  {perror( argv[1] );
   exit( 2 );
  }

 _splitpath( argv[ 1 ], outfd, outfdir, outfn, outfext );
 if( !strcmp( outfext, ".TXT" ) )
  {printf( "**Error** Cannot convert '%s%s' because of its' extension: '%s'",
                                                outfn, outfext, outfext );
   puts( "\nTo correct rename it and try again" );
   exit( 1 );
  }

 _makepath( outfname, outfd, outfdir, outfn, ".TXT" );
 if( ( out = fopen( outfname, "wb" ) ) == NULL )
  {perror( outfname );
   exit( 1 );
  }

 if( ( c = getc(in) ) != 0x5A )
  {fprintf(out,"Mode : ");
   if( c == 0x31 )
     c = getc(in);
   if( c == 0 )
     fputs("COMP\x0D\x0A", out);
   else
     if( c == 0x40 )
       fputs("BASE-N\x0D\x0A", out);
     else
      {if( (c & 0x7D) == 0x10 )
        {fputs("SD\ngraph: ", out);

         if( c & 0x80 )
           fputs("DRAW\x0D\x0A", out);
         else
           fputs("NON-\x0D\x0A", out);

         fputs("data : ", out);

         if( c & 0x02 )
           fputs("STO\x0D\x0A", out);
         else
           fputs("NON-\x0D\x0A", out);
        }
       else
         if( (c & 0x7D) == 0x20 )
          {fputs("REG\ngraph: ", out);

           if( c & 0x80 )
             fputs("DRAW\x0D\x0A", out);
           else
             fputs("NON-\x0D\x0A", out);

           fputs("data : ", out);

           if( c & 0x02 )
             fputs("STO\x0D\x0A", out);
           else
             fputs("NON-\x0D\x0A", out);
          }
         else
           fputs("Unknown\x0D\x0A", out);
      }
  }
 else
  {fputs("ALL PROGRAMS.\x0D\x0A", out);
   for( i = 0; i < 5*38; i++)
     getc(in);
   d = 0xFF;
  }

 while((c = getc(in)) != EOF)
  {if( d == 0xFF ^ c == 0xFF )
     fputs( "\x0D\x0A", out );
   d = c;
   fputs( trtable[c], out );
  }

 fputs( "\x0D\x0A\x0D\x0A", out );

 fclose(in);
 fclose(out);
 return 0;
}
