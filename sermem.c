#include "main.h"
#include "sermem.h"
#include <util/delay.h>

void	sermem_init(void)
{
	SM_CTRL_DDR |= (1<<WE) | (1<<OE) | (1<<MR) | (1<<CP);	// set control ports to output
	sermem_reset();	// set counter to addr 0
}	// end of sermem_init

void	sermem_reset(void)
{
	SM_CTRL_OUT |= (1<<MR);	//put reset high
	_delay_us(1);			//wait a little time
	SM_CTRL_OUT &= ~(1<<MR);	// put reset low, i.e. release counter 
}	// end of sermem_reset

uint8_t	sermem_readByte(void)
{
uint8_t x;
	SM_DATA_DDR = ALLPINSIN;
	SM_CTRL_OUT &= ~(1<<OE);
	_delay_us(0.1);
	x = SM_DATA_IN;
	SM_CTRL_OUT |= (1<<OE);
	// finally increment memory counter
	SM_CTRL_OUT |= (1<<CP);
	SM_CTRL_OUT &= ~(1<<CP);	// CP high to low triggers increment
	return x;
}	// end of sermem_readByte

void	sermem_writeByte(uint8_t x)
{
	SM_DATA_DDR = ALLPINSOUT;
	SM_DATA_OUT = x;	// put byte on data port
	SM_CTRL_OUT &= ~(1<<WE);
	_delay_us(0.1);
	SM_CTRL_OUT |= (1<<WE);
	// finally increment memory counter
	SM_CTRL_OUT |= (1<<CP);
	SM_CTRL_OUT &= ~(1<<CP);	// CP high to low triggers increment
}	// end of sermem_writeByte
