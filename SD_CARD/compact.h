//#########################################################################
// File: compact.h
//
// CompactFlash hardware definitions
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 15.10.2006
//#########################################################################
// Compiler: AVR-GCC 3.4.3
//#########################################################################

#ifndef __COMPACT_FLASH_H	
#define __COMPACT_FLASH_H

//#define CF_DEBUG //activate debug output via printf()

// Some usefull defines
 #define NOP asm volatile ("nop" ::)
 #define sbi(portn, bitn) ((portn)|=(1<<(bitn)))
 #define cbi(portn, bitn) ((portn)&=~(1<<(bitn)))

//CF register adresses
#define CF_IO		0	//IO-Port
#define CF_FEATURES	1	//Errors Out / Features In
#define CF_SECCOUNT	2	//Sectorcount
#define CF_LBA0		3	//LBA 0-7
#define CF_LBA1		4	//LBA 8-15
#define CF_LBA2		5	//LBA 16-23
#define CF_LBA3		6	//LBA 24-27
#define CF_STACOM	7	//Status Out / Command In

//ATAPI commands
#define CF_READ_SEC	(unsigned char)0x20
#define CF_WRITE_SEC    (unsigned char)0x30
#define CF_IDENTIFY     (unsigned char)0xEC

//port where data i/o has to be done
//without pullups on PortA some CF do not work !
//maybe for some CF ATMegas internal pullups are to weak.
//then use 10k external pullups on port and please use short
//cables (max. 10cm)

#if defined (__AVR_ATmega32__) || defined (__AVR_ATmega323__) || defined (__AVR_ATmega161__) || defined (__AVR_ATmega644__)
 #define CF_DATA_DIR_IN()  	DDRA=0x00; PORTA=0xFF; // set io-pins to inputs, Pullups
 #define CF_DATA_DIR_OUT() 	DDRA=0xFF; // set io-pins to outputs
 #define CF_READ_DATA()    	PINA	  // read PIN, ! NOT ! PORT
 #define CF_WRITE_DATA(a)  	PORTA=(a); // write to data port

 #define CF_CONTROL_DDR	DDRB
 #define CF_CONTROL_PORT PORTB 	

 #define CF_ADR_PORT 	PORTB   //Port CF_Adresspins ( D0..3 )
 #define CF_ADR_PIN 	PINB    

 #define CF_CS 		4	//Pin number for CF_CS
 #define CF_CS_PORT 	PORTB   //Port where CF_CS is located

 #define CF_RD 		5	//Pin number for CF_RD
 #define CF_RD_PORT 	PORTB   //Port where CF_RD is located

 #define CF_WR 		6	//Pin number for CF_WR
 #define CF_WR_PORT 	PORTB   //Port where CF_WR is located

#elif defined (__AVR_ATmega128__) || defined (__AVR_ATmega64__)
 #define CF_DATA_DIR_IN()  	DDRA=0x00; PORTA=0xFF; // set io-pins to inputs, Pullups
 #define CF_DATA_DIR_OUT() 	DDRA=0xFF; // set io-pins to outputs
 #define CF_READ_DATA()    	PINA	  // read PIN, ! NOT ! PORT
 #define CF_WRITE_DATA(a)  	PORTA=(a); // write to data port

 #define CF_CONTROL_DDR	DDRF
 #define CF_CONTROL_PORT PORTF 	

 #define CF_ADR_PORT 	PORTF   //Port CF_Adresspins ( D0..3 )
 #define CF_ADR_PIN 	PINF    

 #define CF_CS 		4	//Pin number for CF_CS
 #define CF_CS_PORT 	PORTF   //Port where CF_CS is located

 #define CF_RD 		5	//Pin number for CF_RD
 #define CF_RD_PORT 	PORTF   //Port where CF_RD is located

 #define CF_WR 		6	//Pin number for CF_WR
 #define CF_WR_PORT 	PORTF   //Port where CF_WR is located

#else
#  error "processor type not defined in compact.h"
#endif

#define CF_CS_ON() 	sbi(CF_CS_PORT,CF_CS);
#define CF_CS_OFF() 	cbi(CF_CS_PORT,CF_CS);

#define CF_RD_ON() 	sbi(CF_RD_PORT,CF_RD);
#define CF_RD_OFF() 	cbi(CF_RD_PORT,CF_RD);

#define CF_WR_ON() 	sbi(CF_WR_PORT,CF_WR);
#define CF_WR_OFF() 	cbi(CF_WR_PORT,CF_WR);

//prototypes
extern U32 maxsect;           // last sector on drive

extern unsigned char CFReadSector(unsigned long lba, unsigned char *buf);
extern unsigned char CFWriteSector(unsigned long lba, unsigned char *buf);
extern unsigned char CFIdentify(void);
extern unsigned char CFWaitReady(void);
extern unsigned char CFWaitDrq(void);
extern unsigned char CFReadAdr(unsigned char adr);
extern unsigned char CFRead(void);
extern void CFWriteAdr(unsigned char adr, unsigned char dat);
extern void CFSetAdr(unsigned char adr);

#define ReadSector(a,b) 	CFReadSector((a),(b))
#define WriteSector(a,b) 	CFWriteSector((a),(b))
#define IdentifyMedia()		CFIdentify()

#endif
