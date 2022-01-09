//#########################################################################
// File: serial.c
//
// Aufgeräumte Version. Das Programm ist jetzt wesentlich besser lesbar.
// Dafür sieht es jetzt in serial.h wüst aus :(
//
//#########################################################################
// Last change: 04.07.2007
//#########################################################################
// hk@holger-klabunde.de
// http://www.holger-klabunde.de/index.html
//#########################################################################
// Compiler: AVR-GCC 3.4.5
// Compiler: AVR-GCC 4.1.1
//#########################################################################
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdarg.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "../main.h"

#include "serial.h"

#define RBUFLEN 16 //Pufferlänge für seriellen Empfang

volatile unsigned char rbuf[RBUFLEN]; //Ringpuffer 
volatile unsigned char rcnt, rpos, recbuf;

volatile unsigned char busy;

//#################################################################
//Receive Interruptroutine
//#################################################################
ISR(SIG_UART0_RECEIVE)      /* signal handler for receive complete interrupt */
{
 recbuf= UART0_RECEIVE_REGISTER; //Byte auf jeden Fall abholen, sonst Endlosinterrupt

 /* don't overwrite chars already in buffer */
 if(rcnt < RBUFLEN)  rbuf[(rpos+rcnt++) % RBUFLEN] = recbuf;
}

//#################################################################
//Transmit Interruptroutine
//#################################################################
ISR(SIG_UART0_TRANSMIT)     /* signal handler for transmit complete interrupt */
{
 DISABLE_UART0_TRANSMIT_INT; //ATmega Disable Transmit complete interrupt löschen

 busy=0; //Byte gesendet Flag rücksetzen
}

//#################################################################
unsigned char ser_getc (void)
//#################################################################
{
 unsigned char c;

 while(!rcnt) { };   /* wait for character */

 DISABLE_UART0_RECEIVE_INT; //ATMega Disable Receiveinterrupt

 rcnt--;
 c = rbuf [rpos++];
 if (rpos >= RBUFLEN)  rpos = 0;

 ENABLE_UART0_RECEIVE_INT; //ATmega Enable Receiveinterrupt

 return (c);
}

//#################################################################
void ser_putc(unsigned char c)
//#################################################################
{
//Nicht zu warten ist schlecht !
//Die Zeichenausgabe leidet dann !
  while(busy==1); //warten bis letztes Byte gesendet wurde
   
  ENABLE_UART0_TRANSMIT_INT; //ATmega Enable Transmit complete interrupt erlauben
  UART0_TRANSMIT_REGISTER=c; //Byte in Sendepuffer

  busy = 1;        //Setze Flag Transmit gestartet
}

//#################################################################
// Transmit string from RAM
void ser_puts(unsigned char * s)
//#################################################################
{
 unsigned char c;

   while((c=*s++))
    {
     if(c == '\n') //CR und LF senden bei \n
      {
       ser_putc(0x0D); //CR
       ser_putc(0x0A); //LF
      }
     else ser_putc(c);
    }
}

//##############################################
// Transmit string from FLASH
void ser_puts_p(char const *s)
//##############################################
{
 unsigned char c;

 while((c=pgm_read_byte(s++)))
    {
     if(c == '\n') //CR und LF senden bei \n
      {
       ser_putc(0x0D); //CR
       ser_putc(0x0A); //LF
      }
     else ser_putc(c);
    }
}

//void new_line( void) {	sendstring("\n\r"); }

//#################################################################
void ser_init(void)
//#################################################################
{
  rcnt = rpos = 0;  /* init buffers */
  busy = 0;

  sbi(TX0_DDR,TX0_BIT);  //TxD output
  sbi(TX0_PORT,TX0_BIT); //set TxD to 1

  /* enable RxD/TxD and ints  */
  UART0_CONFIGURE1; // See serial.h
  UART0_CONFIGURE2; // See serial.h

  UART0_BAUD_REGISTER_HIGH=(unsigned char)(UART_BAUD_SELECT>>8); /* set baudrate*/
  UART0_BAUD_REGISTER_LOW=(unsigned char)(UART_BAUD_SELECT); /* set baudrate*/

}

//#################################################################
//Zeigt ein Byte im HexCode an
void ser_puthex(unsigned char by)
//#################################################################
{
 unsigned char buff;

 buff=by>>4; //Highnibble zuerst
 if(buff<10) buff+='0'; //ASCII Code erzeugen
 else buff+=0x37;        //Großbuchstaben
 ser_putc(buff);

 buff=by&0x0f; //Danach das Lownibble
 if(buff<10) buff+='0'; //ASCII Code erzeugen
 else buff+=0x37;        //Großbuchstaben
 ser_putc(buff);
}
