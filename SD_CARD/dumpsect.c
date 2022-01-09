//###########################################################
// File: dumpsect.c
//
// Gibt ein HEX und ASCII Listing aus.
// Jede Zeile enthält 16 Bytes.
//
//#########################################################################
// Last change: 06.03.2004
//#########################################################################
// hk@holger-klabunde.de
// http://www.holger-klabunde.de/index.html
//#########################################################################

#include "serial.h"

//######################################################
void DumpSector(unsigned char *buf)
//######################################################
{
 unsigned int i;
 unsigned char by,j;
 
 for(i=0; i<512; i+=16)
  {
   for(j=0; j<16; j++) { by=buf[i+j]; ser_puthex(by); } //HEX-Dump
   ser_putc(' ');
   ser_putc(' ');

   for(j=0; j<16; j++) //ASCII-Dump
    {
     by=buf[i+j];
     if(by<0x20) by='.';
     ser_putc(by);
    }

   ser_putc(0x0d);
   ser_putc(0x0a);

  }
}

