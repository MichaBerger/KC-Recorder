#include "main.h"

uint8_t 			serbuffer[SERBUFSIZE];
uint16_t			serRdPointer;
volatile uint16_t	serInPointer = 0;
volatile int16_t	buf_usage = 0;
volatile uint32_t	rleCounter;
volatile uint32_t	downCounter = 0;
volatile uint32_t	nextRLEword;
volatile uint8_t	reqNextFlag = FALSE;

ISR (USART0_RX_vect)	// serial Interface
{
uint8_t c;
	c = UDR0;			// get one more char from serial line
	serbuffer[serInPointer++] = c;
	serInPointer &= SERBUFMASK;
	buf_usage++;
}	// end of UART ISR


ISR (TIMER0_COMPA_vect)
{
	rleCounter++;	// 32 bit counter for recording
}

ISR (TIMER2_COMPA_vect)
{
	if (downCounter == 0) {
		PORTA ^= 1<<REPLAY;				// issue a signal level change for REPLAY
		downCounter = nextRLEword+1;	// get next RLE countdown number + 1
		reqNextFlag = TRUE;
	}
	downCounter--;	// do this last to avoid an undesired underrun
}

int main(void)
{

// general settings for all ports
	DDRA = ALLPINSIN | (1<<PA4);	// only PLAY is output
	DDRB = ALLPINSIN;	// LCD ports will be initialized separately 
	DDRC = ALLPINSIN;	// port C is input
	DDRD = ALLPINSIN; 
	PORTA = PULLUP & ~((1<<PA2) | (1<<PA4));	// activate internal pull up's except REC, set PLAY to GND
	PORTB = PULLUP;		// activate internal pull up's
	PORTC = PULLUP;		// activate internal pull up's
	PORTD = PULLUP;		// activate internal pull up's

	initCounter();	// start counter but do not yet activate interrupt for it
	initUART();		// prepare UART settings
	lcd_init();		// initialize LCD, cursor off
	sermem_init();	// initialize serial memory
	// output redirect
	FILE uart_output = FDEV_SETUP_STREAM(uart_putChar, NULL, _FDEV_SETUP_WRITE);
	stdout = &uart_output;

	PCICR |= 1<<PCIE3;		// activate pin change INT for port D: 4 buttons	
	PCMSK3 = KEYMASK;		// allow pins PD4..PD7 as interrupt sources
	PCICR |= 1<<PCIE0;		// activate pin change INT for port A: for "record" pin
	// do not yet activate record pin change int

	sei();
	monitor();	// monitor is the main loop
}


void initCounter(void)		// start Counter0 and Counter2 in CTC mode, but do not yet activate interrupt
{
	// 1. the counter for record activities: runs at 88200 /s
	TCCR0A  = 1<<WGM01;		// CTC mode
	TCCR0B |= 1<<CS00;		// No prescaler, counter runs at system clock speed
	OCR0A = REC_CTCCONST - 1;	// set counter limit
	// 2. the counter for replay activities @ 44100 /s
	TCCR2A = 1<<WGM21;		// CTC mode
	TCCR2B |= 1<<CS21;		// prescaler 8, so counter runs at Fi / 8
	OCR2A = PLAY_CTCCONST - 1;	
}

void initUART(void)
{
	// Set baud rate
	UBRR0H = (uint8_t) (USART_UBRR_VALUE>>8);
	UBRR0L = (uint8_t) USART_UBRR_VALUE;
	// Enable receive interrupt, receiver and transmitter
	UCSR0B = (1<<RXCIE0) | (1<<RXEN0) | (1<<TXEN0);
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);	// 8N1 format
}	// end of initUART

int uart_putChar(uint8_t c)
{
	while (!(UCSR0A & (1<<UDRE0)))  // wait until send allowed
	{ 
	}
	UDR0 = c;
	return 0;
}	// end of uart_putChar

uint8_t uart_getChar(void)
{
uint8_t	c;
	while (buf_usage <= 0)
	{ 
	}
	cli();
	c = serbuffer[serRdPointer++];
	serRdPointer &= SERBUFMASK;
	buf_usage--;
	sei();
	return c;
}	// end of uart_getChar
