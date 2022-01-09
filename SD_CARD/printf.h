//#########################################################################
// PRINTF.H
//
//#########################################################################
// Last change: 29.07.2005
//#########################################################################
// Compiler: AVR-GCC 3.4.3
//#########################################################################

#ifndef __PRINTF_H
#define __PRINTF_H

#include <avr/pgmspace.h>

extern void myputchar(unsigned char c);

extern void _puts_P(char const *txt);
#define puts(text)   _puts_P(PSTR(text))

extern void _printf_P (char const *fmt0, ...);
#define printf(format, args...)   _printf_P(PSTR(format) , ## args)

#endif
