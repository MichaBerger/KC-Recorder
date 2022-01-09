//#########################################################################
// Last change: 27.10.2006
//#########################################################################
// Compiler: AVR-GCC 3.4.3
//#########################################################################
#ifndef ___SERIAL_H
#define ___SERIAL_H

#include <avr/pgmspace.h>

#ifndef UART_BAUD_RATE
//#define UART_BAUD_RATE     4800
//#define UART_BAUD_RATE     9600
#define UART_BAUD_RATE    19200
//#define UART_BAUD_RATE    38400
//#define UART_BAUD_RATE   115200  
#endif

#define UART_BAUD_SELECT (F_CPU/(UART_BAUD_RATE*16l)-1)

// Some usefull defines
#ifndef NOP
 #define NOP asm volatile ("nop" ::)
#endif

#define sbi(portn, bitn) ((portn)|=(1<<(bitn)))
#define cbi(portn, bitn) ((portn)&=~(1<<(bitn)))

extern void ser_init(void);
extern void ser_putc(unsigned char c);
extern void ser_puts(unsigned char *s);
extern int ser_gets(unsigned char *s, unsigned char len);
extern unsigned char ser_getc(void);
extern void ser_puthex(unsigned char by);

extern volatile unsigned char rcnt, rpos;
extern volatile unsigned char busy;

extern void ser_puts_p(char const *txt);
#define ser_puts_P(text)   ser_puts_p(PSTR(text))

// Anpassungen an die vielen bescheuerten Namensänderungen die ATMEL
// quasi von ATMega zu ATMega in den Datenblättern vornimmt.
// Es könnte soo einfach sein.
#if defined (__AVR_ATmega32__) || defined (__AVR_ATmega323__)
  #define TX0_BIT	1
  #define TX0_PORT	PORTD
  #define TX0_DDR	DDRD

  #define SIG_UART0_RECEIVE		SIG_UART_RECV
  #define SIG_UART0_TRANSMIT		SIG_UART_TRANS
  #define UART0_RECEIVE_REGISTER        UDR
  #define UART0_TRANSMIT_REGISTER       UDR
  #define ENABLE_UART0_TRANSMIT_INT	sbi(UCSRB,TXEN)  //Wieso nehme ich hier nicht TXCIE ?
  #define DISABLE_UART0_TRANSMIT_INT	cbi(UCSRB,TXEN)  //Es gibt einen Grund, aber habe vergessen 
  #define ENABLE_UART0_RECEIVE_INT	sbi(UCSRB,RXCIE)
  #define DISABLE_UART0_RECEIVE_INT	cbi(UCSRB,RXCIE)
  #define UART0_BAUD_REGISTER_HIGH	UBRRH
  #define UART0_BAUD_REGISTER_LOW	UBRRL
  #define UART0_CONFIGURE1		UCSRB= (1<<RXCIE) | (1<<TXCIE) | (1<<RXEN) | (1<<TXEN)
  #define UART0_CONFIGURE2		UCSRC= (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0) //8 Bit,1 Stop, no parity
  
#elif  defined (__AVR_ATmega161__)
  #define TX0_BIT	1
  #define TX0_PORT	PORTD
  #define TX0_DDR	DDRD

  #define SIG_UART0_RECEIVE		SIG_UART0_RECV
  #define SIG_UART0_TRANSMIT		SIG_UART0_TRANS
  #define UART0_RECEIVE_REGISTER        UDR0
  #define UART0_TRANSMIT_REGISTER       UDR0
  #define ENABLE_UART0_TRANSMIT_INT	sbi(UCSR0B,TXEN)
  #define DISABLE_UART0_TRANSMIT_INT	cbi(UCSR0B,TXEN);
  #define ENABLE_UART0_RECEIVE_INT	sbi(UCSR0B,RXCIE)
  #define DISABLE_UART0_RECEIVE_INT	cbi(UCSR0B,RXCIE)
  #define UART0_BAUD_REGISTER_HIGH	UBRRH
  #define UART0_BAUD_REGISTER_LOW	UBRR0 // Ich glaub es nicht. Wieso kein L statt 0 ?
  #define UART0_CONFIGURE1		UCSR0B= (1<<RXCIE) | (1<<TXCIE) | (1<<RXEN) | (1<<TXEN)
  #define UART0_CONFIGURE2		NOP //Ho, ho, hoo. 8 Bit,1 Stop, no parity

#elif defined (__AVR_ATmega168__) || defined (__AVR_ATmega48__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega644__) || defined (__AVR_ATmega1284P__)
  #define TX0_BIT	1
  #define TX0_PORT	PORTD
  #define TX0_DDR	DDRD

  #define SIG_UART0_RECEIVE		SIG_USART_RECV
  #define SIG_UART0_TRANSMIT		SIG_USART_TRANS
  #define UART0_RECEIVE_REGISTER        UDR0  // UDR hat ne Null hinten, aber USART nicht. Ich schmeiß mich weg !!!
  #define UART0_TRANSMIT_REGISTER       UDR0
  #define ENABLE_UART0_TRANSMIT_INT	sbi(UCSR0B,TXEN0);
  #define DISABLE_UART0_TRANSMIT_INT	cbi(UCSR0B,TXEN0);
  #define ENABLE_UART0_RECEIVE_INT	sbi(UCSR0B,RXCIE0)
  #define DISABLE_UART0_RECEIVE_INT	cbi(UCSR0B,RXCIE0)
  #define UART0_BAUD_REGISTER_HIGH	UBRR0H
  #define UART0_BAUD_REGISTER_LOW	UBRR0L
  #define UART0_CONFIGURE1		UCSR0B= (1<<RXCIE0) | (1<<TXCIE0) | (1<<RXEN0) | (1<<TXEN0)
  					// Was sollen jetzt die 0en hinter den Bits ? 
  #define UART0_CONFIGURE2		UCSR0C= (1<<UCSZ01) | (1<<UCSZ00) //8 Bit,1 Stop, no parity

#elif defined (__AVR_ATmega8__)
  #define TX0_BIT	1
  #define TX0_PORT	PORTD
  #define TX0_DDR	DDRD

  #define SIG_UART0_RECEIVE		SIG_UART_RECV
  #define SIG_UART0_TRANSMIT		SIG_UART_TRANS
  #define UART0_RECEIVE_REGISTER        UDR
  #define UART0_TRANSMIT_REGISTER       UDR
  #define ENABLE_UART0_TRANSMIT_INT	sbi(UCSRB,TXEN);
  #define DISABLE_UART0_TRANSMIT_INT	cbi(UCSRB,TXEN);
  #define ENABLE_UART0_RECEIVE_INT	sbi(UCSRB,RXCIE)
  #define DISABLE_UART0_RECEIVE_INT	cbi(UCSRB,RXCIE)
  #define UART0_BAUD_REGISTER_HIGH	UBRRH
  #define UART0_BAUD_REGISTER_LOW	UBRRL
  #define UART0_CONFIGURE1		UCSRB= (1<<RXCIE) | (1<<TXCIE) | (1<<RXEN) | (1<<TXEN)
  #define UART0_CONFIGURE2		UCSRC= (1<<UCSZ1) | (1<<UCSZ0) //8 Bit,1 Stop, no parity

#elif defined (__AVR_ATmega128__) || defined (__AVR_ATmega64__)
  #define TX0_BIT	1
  #define TX0_PORT	PORTE
  #define TX0_DDR	DDRE

  #define SIG_UART0_RECEIVE		SIG_UART0_RECV
  #define SIG_UART0_TRANSMIT		SIG_UART0_TRANS
  #define UART0_RECEIVE_REGISTER        UDR0
  #define UART0_TRANSMIT_REGISTER       UDR0
  #define ENABLE_UART0_TRANSMIT_INT	sbi(UCSR0B,TXEN)
  #define DISABLE_UART0_TRANSMIT_INT	cbi(UCSR0B,TXEN);
  #define ENABLE_UART0_RECEIVE_INT	sbi(UCSR0B,RXCIE)
  #define DISABLE_UART0_RECEIVE_INT	cbi(UCSR0B,RXCIE)

  #define UART0_BAUD_REGISTER_HIGH	UBRR0H
  #define UART0_BAUD_REGISTER_LOW	UBRR0L
  #define UART0_CONFIGURE1		UCSR0B= (1<<RXCIE) | (1<<TXCIE) | (1<<RXEN) | (1<<TXEN)
  #define UART0_CONFIGURE2		UCSR0C= (1<<UCSZ1) | (1<<UCSZ0) //8 Bit,1 Stop, no parity

#else
#  error "processor type not defined in serial.h"
#endif

#endif
