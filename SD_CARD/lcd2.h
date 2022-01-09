//#########################################################################
// File: LCD.H
//
//#########################################################################
// Last change: 13.12.2007
//#########################################################################
// Compiler: AVR-GCC 4.1.1
//#########################################################################
//@{

#ifndef __LCD_H
#define __LCD_H

// You should use only ONE of these !
#define HD44780
//#define KS073
//#define KS066		//not tested
//#define KS070		//not tested
// Todo: 1:1 sequences from datasheet for each device

#define LCD_LINES	2
#define LCD_ROWS	40
// Note: If you have a 1x16 display, you may have to use LCD_LINES 2 and LCD_ROWS 8.
// Most 1x16 displays are 2x8 displays organized in one line.

//#define LCD_BUSY_CHECK 	  // not implemented, RW has to be connected to ground
//#define LCD_CONTROLLERS	1 // not implemented, 2 for 4x40 LCD displays

// You can use only ONE of these !
//#define INTERFACE_4_BIT_MIX		// You can use every free pin for the data lines
//#define INTERFACE_4_BIT_FIX_LOW       // DB4..DB7 of LCD connected to D0..D3 of uC port
//#define INTERFACE_4_BIT_FIX_HIGH	// DB4..DB7 of LCD connected to D4..D7 of uC port
//#define INTERFACE_4_BIT_FIX_MID	// DB4..DB7 of LCD connected to D2..D5 of uC port
#define INTERFACE_8_BIT
//#define INTERFACE_8_BIT_MIX   // not implemented
//#define INTERFACE_SERIAL 	// not implemented
//#define INTERFACE_SPI_HARD    // not implemented
//#define INTERFACE_SPI_SOFT    // not implemented
//#define INTERFACE_I2C_HARD    // not implemented, PCF8574
//#define INTERFACE_I2C_SOFT    // not implemented, PCF8574
//#define INTERFACE_4094   // not implemented, TTL/CMOS shift register like HCF4094


//#define EADIP204     //EADIP204 Zeichensatzkonvertierungen
//#define CUSTOM_TABLE //Eigene Tabelle. In ConvertChar() selbst definieren
//Wenn beide nicht definiert werden wird eine Standardkonvertierung benutzt

#if defined (INTERFACE_4_BIT_MIX) || defined (INTERFACE_4_BIT_FIX_LOW) || defined (INTERFACE_4_BIT_FIX_HIGH) || defined (INTERFACE_4_BIT_FIX_MID)
 #define INTERFACE_4_BIT
#endif

// Some usefull defines
#define NOP	 asm volatile ("nop" ::)
#define sbi(portn, bitn) ((portn)|=(1<<(bitn)))
#define cbi(portn, bitn) ((portn)&=~(1<<(bitn)))

//######################################################################
// Ein paar vordefinierte Testkonfigurationen für meine Testboards
//######################################################################
#if defined (__AVR_ATmega32__) || defined (__AVR_ATmega323__) || defined (__AVR_ATmega161__) || defined (__AVR_ATmega644__) || defined (__AVR_ATmega1284P__)
#ifdef INTERFACE_8_BIT
 #define LCD_RS_BIT	5 //LCD Register Select Pin
 #define LCD_RS_PORT	PORTD
 #define LCD_RS_DDR	DDRD
 #define LCD_E_BIT	4 //LCD Chip Select Pin
 #define LCD_E_PORT	PORTD
 #define LCD_E_DDR	DDRD

 #define LCD_DATA_PORT	PORTC //LCD Data Port
 #define LCD_DATA_DDR	DDRC
 #define LCD_DATA_PIN	PINC
#endif

#ifdef INTERFACE_4_BIT_FIX_LOW
#endif

#ifdef INTERFACE_4_BIT_FIX_HIGH
 #define LCD_RS_BIT	2 //LCD Register Select Pin
 #define LCD_RS_PORT	PORTA
 #define LCD_RS_DDR	DDRA

 #define LCD_E_BIT	3 //LCD Chip Select Pin
 #define LCD_E_PORT	PORTA
 #define LCD_E_DDR	DDRA

 #define LCD_DATA_PORT	PORTA //LCD Data Port
 #define LCD_DATA_DDR	DDRA
 #define LCD_DATA_PIN	PINA
#endif

#ifdef INTERFACE_4_BIT_FIX_MID
#endif

#ifdef INTERFACE_4_BIT_MIX
 #define LCD_RS_BIT	2 //LCD Register Select Pin
 #define LCD_RS_PORT	PORTA
 #define LCD_RS_DDR	DDRA

 #define LCD_E_BIT	3 //LCD Chip Select Pin
 #define LCD_E_PORT	PORTA
 #define LCD_E_DDR	DDRA

 #define LCD_D4_BIT	4
 #define LCD_D4_DDR	DDRA
 #define LCD_D4_PORT	PORTA

 #define LCD_D5_BIT	5
 #define LCD_D5_DDR	DDRA
 #define LCD_D5_PORT	PORTA

 #define LCD_D6_BIT	6
 #define LCD_D6_DDR	DDRA
 #define LCD_D6_PORT	PORTA

 #define LCD_D7_BIT	7
 #define LCD_D7_DDR	DDRA
 #define LCD_D7_PORT	PORTA
#endif

#elif defined (__AVR_ATmega128__) || defined (__AVR_ATmega64__)

#ifdef INTERFACE_8_BIT
 #define LCD_RS_BIT	4 //LCD Register Select Pin
 #define LCD_RS_PORT	PORTD
 #define LCD_RS_DDR	DDRD
 #define LCD_E_BIT	5 //LCD Chip Select Pin
 #define LCD_E_PORT	PORTD
 #define LCD_E_DDR	DDRD

 #define LCD_DATA_PORT	PORTC //LCD Data Port
 #define LCD_DATA_DDR	DDRC
 #define LCD_DATA_PIN	PINC
#endif

#ifdef INTERFACE_4_BIT_FIX_LOW
#endif

#ifdef INTERFACE_4_BIT_FIX_HIGH
#endif

#ifdef INTERFACE_4_BIT_FIX_MID
#endif

#ifdef INTERFACE_4_BIT_MIX
 #define LCD_RS_BIT	2 //LCD Register Select Pin
 #define LCD_RS_PORT	PORTC
 #define LCD_RS_DDR	DDRC

 #define LCD_E_BIT	3 //LCD Chip Select Pin
 #define LCD_E_PORT	PORTC
 #define LCD_E_DDR	DDRC

 #define LCD_D4_BIT	4
 #define LCD_D4_DDR	DDRC
 #define LCD_D4_PORT	PORTC

 #define LCD_D5_BIT	5
 #define LCD_D5_DDR	DDRC
 #define LCD_D5_PORT	PORTC

 #define LCD_D6_BIT	6
 #define LCD_D6_DDR	DDRC
 #define LCD_D6_PORT	PORTC

 #define LCD_D7_BIT	7
 #define LCD_D7_DDR	DDRC
 #define LCD_D7_PORT	PORTC
#endif

#elif defined (__AVR_ATmega168__) || defined (__AVR_ATmega48__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega8__)

#ifdef INTERFACE_8_BIT
// 8 Bit Modus ist Blödsinn bei diesen kleinen Controllern
#endif

#ifdef INTERFACE_4_BIT_FIX_LOW
#endif

#ifdef INTERFACE_4_BIT_FIX_HIGH
#endif

#ifdef INTERFACE_4_BIT_FIX_MID
 #define LCD_RS_BIT	0 //LCD Register Select Pin
 #define LCD_RS_PORT	PORTC
 #define LCD_RS_DDR	DDRC

 #define LCD_E_BIT	1 //LCD Chip Select Pin
 #define LCD_E_PORT	PORTC
 #define LCD_E_DDR	DDRC

 #define LCD_DATA_PORT	PORTC //LCD Data Port
 #define LCD_DATA_DDR	DDRC
 #define LCD_DATA_PIN	PINC
#endif

#ifdef INTERFACE_4_BIT_MIX
//defines for myavr Board
 #define LCD_RS_BIT	2 //LCD Register Select Pin
 #define LCD_RS_PORT	PORTD
 #define LCD_RS_DDR	DDRD

 #define LCD_E_BIT	3 //LCD Chip Select Pin
 #define LCD_E_PORT	PORTD
 #define LCD_E_DDR	DDRD

 #define LCD_D4_BIT	4
 #define LCD_D4_DDR	DDRD
 #define LCD_D4_PORT	PORTD

 #define LCD_D5_BIT	5
 #define LCD_D5_DDR	DDRD
 #define LCD_D5_PORT	PORTD

 #define LCD_D6_BIT	6
 #define LCD_D6_DDR	DDRD
 #define LCD_D6_PORT	PORTD

 #define LCD_D7_BIT	7
 #define LCD_D7_DDR	DDRD
 #define LCD_D7_PORT	PORTD
#endif

#else
#  error "processor type not defined in lcd.h"
#endif

#define LCD_E_ON 	sbi(LCD_E_PORT, LCD_E_BIT)
#define LCD_E_OFF	cbi(LCD_E_PORT, LCD_E_BIT)
#define LCD_RS_ON	sbi(LCD_RS_PORT, LCD_RS_BIT)
#define LCD_RS_OFF	cbi(LCD_RS_PORT, LCD_RS_BIT)

extern void LCDInit(void);
extern void LCDCls(void);
extern void LCDPos(unsigned char, unsigned char);
extern void LCDSetCGRAM(unsigned char adr, unsigned char *buf);
extern void LCDWriteByte(unsigned char);
extern void LCDWriteNibble(unsigned char);
extern void LCDWrite(char *);

extern void LCDWrite_p(const char *s);
#define LCDWrite_P(text)   LCDWrite_p(PSTR(text))

extern char ConvertChar(char data);

extern void Delay1ms(unsigned int time);
extern void Delay1us(unsigned int time);

extern void LCDHexChar(unsigned char by);
extern void LCDHexInt(unsigned int by);
extern void LCDHexLong(unsigned long l);

// Some security checks !
#if defined (HD44780) && defined (KS073)
 #error "Define HD44780 or KS073 only in lcd.h, NOT both !"
#endif

#if defined (HD44780) && defined (KS066)
 #error "Define HD44780 or KS066 only in lcd.h, NOT both !"
#endif

#if defined (KS066) && defined (KS073)
 #error "Define KS066 or KS073 only in lcd.h, NOT both !"
#endif

#if !defined (HD44780) && !defined (KS066) && !defined (KS070) && !defined (KS073)
 #error "Define at least HD44780 or KS066 or KS070 or KS073 in lcd.h !"
#endif

#if defined (INTERFACE_4_BIT) && defined (INTERFACE_8_BIT)
 #error "Define INTERFACE_4_BIT or INTERFACE_8_BIT only in lcd.h, NOT both !"
#endif

#endif
//@}
