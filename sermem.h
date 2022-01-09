#ifndef SERMEM_H
#define SERMEM_H

#define SM_DATA_DDR		DDRC
#define SM_DATA_OUT		PORTC
#define	SM_DATA_IN		PINC
#define SM_CTRL_DDR		DDRA
#define SM_CTRL_OUT		PORTA
#define WE			PA0
#define OE			PA1
#define MR			PA6
#define CP			PA7
#define ALLPINSIN		0b00000000	// entire Atmel port to Input
#define ALLPINSOUT		0b11111111	// entire Atmel port to Output

#include <avr/io.h>

void	sermem_init(void);		// intialize ports etc., must be called first
void	sermem_reset(void);		// reset RAM counter to begin of RAM
uint8_t	sermem_readByte(void);		// read byte and increment
void	sermem_writeByte(uint8_t);	// write byte and increment

#endif // SERMEM_H
