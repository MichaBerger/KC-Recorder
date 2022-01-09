#ifndef MAIN_H_
#define MAIN_H_

#define VERSION		"0.9e 27-Oct-2015"
#define F_CPU 18432000ul
#define USART_BAUD 9600ul
#define USART_UBRR_VALUE ((F_CPU/(USART_BAUD<<4))-1)
#define SERBUFSIZE 1024		// must be a 2^n number
#define SERBUFMASK (SERBUFSIZE - 1)
#define ALLPINSOUT	0b11111111	// entire Atmel port to Output
#define ALLPINSIN	0b00000000	// entire Atmel port to Input
#define PULLUP		0b11111111
#define CRLF		"\r\n"
#define BYTES_LINE	16			// bytes per line for RAM dump
#define ERR 		-1
#define MEMSIZE		160			// Ram Buffer for direct access to file header
#define REPLAY		PA4			// replay is pin PA4
#define AUX			(PINA & (1<<PA3))
#define REC			(PINA & (1<<PA2))
#define KEYMASK		0xF0		// mask for key input pins

#define REC_CTCCONST	209			// for 88200 /s sample rate						
#define PLAY_CTCCONST	52			// for 44100 /s replay

#define CTC0_INT_ON		TIMSK0 |= (1<<OCIE0A)	// switch on record CTC interrupt
#define CTC0_INT_OFF	TIMSK0 &= ~(1<<OCIE0A)	// switch off record CTC interrupt
#define CTC2_INT_ON		TIMSK2 |= (1<<OCIE2A)	// switch on replay CTC interrupt
#define CTC2_INT_OFF	TIMSK2 &= ~(1<<OCIE2A)	// switch off replay CTC interrupt

#define REC_INT_ON	PCMSK0 |= (1<<PA2)		// enable pin change int. for record pin
#define REC_INT_OFF	PCMSK0 &= ~(1<<PA2)		// disable it

// LED control actions
#define REC_LED_ON	DDRD |= 1<<PD6; PORTD &= ~(1<<PD6)
#define REC_LED_OFF	DDRD &= ~(1<<PD6); PORTD |= 1<<PD6
#define PLAY_LED_ON	DDRD |= 1<<PD4; PORTD &= ~(1<<PD4)
#define PLAY_LED_OFF DDRD &= ~(1<<PD4); PORTD |= 1<<PD4
// keys for primary menu:
#define REC_KEY		PD6
#define PLAY_KEY	PD4
#define BREAK_KEY	PD5
#define DIR_KEY		PD7
#define TRUE		0xFF
#define FALSE		0

// KC Time Constants:
#define TIMECONST1	11		// Zero Bit
#define TIMECONST2	20		// One Bit and pretune
#define TIMECONST3	39		// Byte Separator

// used libraries
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lcd.h"
#include "sermem.h"

// function prototypes
void 	initCounter(void);
void 	initUART(void);
int  	uart_putChar(uint8_t c);
uint8_t	uart_getChar(void);
void 	monitor(void);
void 	dispFileName(char *filename);
void 	eraseRAM(void);
void 	writeFile_SD(char *filename);
void 	Bin2Hex(char *here,uint16_t number,uint8_t digits);


// variable declarations
extern 			uint8_t 	serbuffer[];	// buffer for ser. communication
extern volatile int16_t		buf_usage;
extern volatile uint16_t 	serInPointer;
extern volatile uint32_t	rleCounter;		// 32 bit counter for recording
extern volatile uint32_t	downCounter;	// 32 bit counter for replay
extern volatile uint32_t	nextRLEword;
extern volatile uint8_t		reqNextFlag;
extern			uint32_t	RAMcounter;		// counter for current record memory position
extern			uint8_t		recMem[];	
extern			uint8_t		RLEmode;
extern 			uint16_t 	numFiles;	
extern			char		filename[];

#endif	// MAIN_H_

