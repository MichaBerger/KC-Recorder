/*! \file "mmc_spi.h" \brief MMC/SD-Definitions */
///	\ingroup MMC
///	\defgroup MMC MMC/SD-Functions (mmc_spi.h)
//#########################################################################
// File: mmc_spi.h
//
// MMC MultiMediaCard and SD SecureDigital definitions for SPI protocol
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 20.10.2008
//#########################################################################
// Compiler: AVR-GCC 4.1.2
//#########################################################################
//@{

#ifndef __MMC_CARD_SPI_H
#define __MMC_CARD_SPI_H

#define GCC_VERSION (__GNUC__ * 10000 \
                               + __GNUC_MINOR__ * 100 \
                               + __GNUC_PATCHLEVEL__)
          /* Test for GCC > 4.1.2 */
//          #if GCC_VERSION > 40102

// Use only ONE of these ! Not ready yet. Don't use SPI1.
#define USE_SPI0
//#define USE_SPI1

#define SPI_PRESCALER		2	//Choose SPI Speed here. You can use 2,4,8,16,32,64,128. 2 is fastest

// Use ONE of these definitions only !
//#define STANDARD_SPI_READ   // No optimizations
#define FAST_SPI_READ       // Optimizations
//#define HYPER_SPI_READ        // Very fast. Use this with AVR/ATMEGA only !
                              // It's very dangerous to use this !
                              // SPI clock has to be F_CPU/2.
                              // THIS IS FOR TESTING ONLY !
                              // Don't use for development !
                              // Next compiler version may give you a kick in the ass.

// Use ONE of these definitions only !
//#define STANDARD_SPI_WRITE  // No optimizations
#define FAST_SPI_WRITE      // Optimizations
//#define HYPER_SPI_WRITE       // Very fast. Use this with AVR/ATMEGA only !
                              // It's very dangerous to use this !
                              // SPI clock has to be F_CPU/2.
                              // THIS IS FOR TESTING ONLY !
                              // Don't use for development !
                              // Next compiler version may give you a kick in the ass.

//#define CHECK_MAXSECTOR  // Maximum sector number check in ReadSector(), WriteSector()
                           // Not needed if YOU check it yourself before calling these routines ;)

//#define BUSY_BEFORE_CMD     // Don't use this ! Resending sectors does not work with it !
			      // Busy check after writing is done before sending a new command.
                              // So write buffer can be filled while card is busy. This gives
                              // maximum speed !
                              // Does only work for my MMC cards. SD cards don't like it !
                              // If you use SD cards only, you should not define this.

//#define MMC_DEBUG_MAX_BUSY_WRITE //activate counting maximum busy time for writes (timer ISR required !)
//#define MMC_DEBUG_IDENTIFY //activate debug output for MMCIdentify() via printf()
//#define MMC_DEBUG_SECTORS_READ //activate debug output of sectornumbers via printf()
//#define MMC_DEBUG_SECTORS_WRITE //activate debug output of sectornumbers via printf()
//#define MMC_DEBUG_COMMANDS //activate debug output via printf()
//#define MMC_DEBUG_CMD0_TIMEOUT //activate debug output via printf()
//#define MMC_DEBUG_CMD1_TIMEOUT //activate debug output via printf()

// Some usefull defines
#define NOP asm volatile ("nop" ::)
#define sbi(portn, bitn) ((portn)|=(1<<(bitn)))
#define cbi(portn, bitn) ((portn)&=~(1<<(bitn)))

  //makes it easier to switch to another hardware
#define SPI_CTRL_REGISTER	SPCR
#define SPI_STATUS_REGISTER	SPSR
#define SPI_DATA_REGISTER	SPDR
#define SPI_STATUS_IF_BIT	SPIF

#define SPI_WRITE(a)	 	{ SPI_DATA_REGISTER=(a); }
#define SPI_WAIT() 		{ while(! ( SPI_STATUS_REGISTER & (1<<SPI_STATUS_IF_BIT) ) ); }

#if defined (__AVR_ATmega32__) || defined (__AVR_ATmega323__) || defined (__AVR_ATmega161__) || defined (__AVR_ATmega163__)

 #define MMC_CS_BIT	4	//Pin number for MMC_CS
 #define MMC_CS_PORT 	PORTB   //Port where MMC_CS is located
 #define MMC_CS_DDR 	DDRB    //Port direction register where MMC_CS is located

 #define MMC_SCK_BIT	7
 #define MMC_SCK_PORT   PORTB
 #define MMC_SCK_DDR    DDRB

 #define MMC_MISO_BIT	6
 #define MMC_MISO_PORT  PORTB
 #define MMC_MISO_DDR   DDRB

 #define MMC_MOSI_BIT	5
 #define MMC_MOSI_PORT  PORTB
 #define MMC_MOSI_DDR   DDRB

#elif defined (__AVR_ATmega128__) || defined (__AVR_ATmega64__) || defined (__AVR_AT90CAN128__)

 #define MMC_CS_BIT	0	//Pin number for MMC_CS
 #define MMC_CS_PORT 	PORTB   //Port where MMC_CS is located
 #define MMC_CS_DDR 	DDRB    //Port direction register where MMC_CS is located

 #define MMC_SCK_BIT	1
 #define MMC_SCK_PORT   PORTB
 #define MMC_SCK_DDR    DDRB

 #define MMC_MISO_BIT	3
 #define MMC_MISO_PORT  PORTB
 #define MMC_MISO_DDR   DDRB

 #define MMC_MOSI_BIT	2
 #define MMC_MOSI_PORT  PORTB
 #define MMC_MOSI_DDR   DDRB

#elif defined (__AVR_ATmega168__) || defined (__AVR_ATmega8__) || defined (__AVR_ATmega88__)
 #define MMC_CS_BIT	2	//Pin number for MMC_CS
 #define MMC_CS_PORT 	PORTB   //Port where MMC_CS is located
 #define MMC_CS_DDR 	DDRB    //Port direction register where MMC_CS is located

 #define MMC_SCK_BIT	5
 #define MMC_SCK_PORT   PORTB
 #define MMC_SCK_DDR    DDRB

 #define MMC_MISO_BIT	4
 #define MMC_MISO_PORT  PORTB
 #define MMC_MISO_DDR   DDRB

 #define MMC_MOSI_BIT	3
 #define MMC_MOSI_PORT  PORTB
 #define MMC_MOSI_DDR   DDRB

#elif defined (__AVR_ATmega644__) || defined (__AVR_ATmega1284P__)

#ifdef USE_SPI0

#if GCC_VERSION < 40102
  #define SPCR	SPCR0
  #define SPIE	SPIE0
  #define SPE	SPE0
  #define DORD	DORD0
  #define MSTR	MSTR0
  #define CPOL	CPOL0
  #define CPHA	CPHA0
  #define SPR1	SPR01
  #define SPR0	SPR00

  #define SPSR	SPSR0
  #define SPIF	SPIF0
  #define WCOL	WCOL0
  #define SPI2X	SPI2X0

  #define SPDR	SPDR0
#endif //GCC_VERSION < 40102


  #define MMC_CS_BIT	4	//Pin number for MMC_CS
  #define MMC_CS_PORT 	PORTB   //Port where MMC_CS is located
  #define MMC_CS_DDR 	DDRB    //Port direction register where MMC_CS is located

  #define MMC_SCK_BIT	7
  #define MMC_SCK_PORT   PORTB
  #define MMC_SCK_DDR    DDRB

  #define MMC_MISO_BIT	6
  #define MMC_MISO_PORT  PORTB
  #define MMC_MISO_DDR   DDRB

  #define MMC_MOSI_BIT	5
  #define MMC_MOSI_PORT  PORTB
  #define MMC_MOSI_DDR   DDRB
 #endif

/*
 #ifdef USE_SPI1
  #define SPCR	SPCR1
  #define SPIE	SPIE1
  #define SPE	SPE1
  #define DORD	DORD1
  #define MSTR	MSTR1
  #define CPOL	CPOL1
  #define CPHA	CPHA1
  #define SPR1	SPR11
  #define SPR0	SPR10

  #define SPSR	SPSR1
  #define SPIF	SPIF1
  #define WCOL	WCOL1
  #define SPI2X	SPI2X1

  #define SPDR	SPDR1

  #  error "SPI1 not ready yet in mmc_spi.h"
 #endif
*/

#else
#  error "processor type not defined in mmc_spi.h"
#endif

#define MMC_CS_ON() 	sbi(MMC_CS_PORT,MMC_CS_BIT);
#define MMC_CS_OFF() 	cbi(MMC_CS_PORT,MMC_CS_BIT);

// MMC/SD commands
#define MMC_CMD0		(U8)(0x40 + 0)
#define MMC_CMD1		(U8)(0x40 + 1)
#define SD_CMD8			(U8)(0x40 + 8)
#define MMC_READ_CSD		(U8)(0x40 + 9)
#define MMC_READ_CID		(U8)(0x40 + 10)
#define MMC_STOP_TRANSMISSION	(U8)(0x40 + 12)
#define MMC_SEND_STATUS		(U8)(0x40 + 13)
#define MMC_SET_BLOCKLEN	(U8)(0x40 + 16)
#define MMC_READ_BLOCK		(U8)(0x40 + 17)
#define MMC_READ_MULTI_BLOCK	(U8)(0x40 + 18)
#define MMC_WRITE_BLOCK		(U8)(0x40 + 24)
#define MMC_WRITE_MULTI_BLOCK	(U8)(0x40 + 25)
#define SD_CMD55		(U8)(0x40 + 55)
#define SD_CMD58		(U8)(0x40 + 58)
#define SD_ACMD41		(U8)(0x40 + 41)

#define DUMMY_WRITE		(U8)(0xFF)
#define START_BLOCK_TOKEN	(U8)(0xFE)

#ifndef BYTE_PER_SEC
 #define BYTE_PER_SEC (U16) 512
#endif

#define CMD0_RETRYS	100

// SPI Response Flags
#define IN_IDLE_STATE	   (U8)(0x01)
#define ERASE_RESET	   (U8)(0x02)
#define ILLEGAL_COMMAND	   (U8)(0x04)
#define COM_CRC_ERROR	   (U8)(0x08)
#define ERASE_ERROR	   (U8)(0x10)
#define ADRESS_ERROR	   (U8)(0x20)
#define PARAMETER_ERROR	   (U8)(0x40)


#define STANDARD_CAPACITY  (U8)0
#define HIGH_CAPACITY      (U8)1

#define UNKNOWN_CARD	(U8)0
#define MMC_CARD	(U8)1
#define SD_CARD		(U8)2
#define SDHC_CARD	(U8)3

//Returncodes for MMCIdentify()
#define MMC_OK			(U8)0
#define CMD0_TIMEOUT		(U8)1
#define CMD1_TIMEOUT		(U8)2
#define ACMD41_TIMEOUT		(U8)3
#define CSD_ERROR		(U8)4
#define TEST_PATTERN_ERROR	(U8)5
#define TEST_VOLTAGE_ERROR	(U8)6
#define SET_BLOCKLEN_ERROR	(U8)7

#ifndef __ASSEMBLER__
//prototypes
extern U32 maxsect;           // last sector on drive

#ifdef MMC_DEBUG_MAX_BUSY_WRITE
 extern volatile U16 countbusy; // you have to count this in a timer ISR
 extern U16 maxbusytime,maxbusyhits;
#endif

extern U8 MMCCommand(U8 command, U32 adress);
extern U8 MMCReadSector(U32 sector, U8 *buf);
extern U8 MMCWriteSector(U32 sector, U8 *buf);
extern U8 MMCIdentify(void);
extern void MMC_IO_Init(void);
extern void GetResponse(U8 *buf, U8 numbytes);
extern void write_spi_hyper(U16 numbytes, U8 *buf);  //spihyper.s
extern void read_spi_hyper(U16 numbytes, U8 *buf); //spihyper.s
extern void write_spi_fast(U16 numbytes, U8 *buf); //spifast.s
extern void read_spi_fast(U16 numbytes, U8 *buf);  //spifast.s

#define ReadSector(a,b) 	MMCReadSector((a),(b))
#define WriteSector(a,b) 	MMCWriteSector((a),(b))
#define IdentifyMedia()		MMCIdentify()
#endif //#ifndef __ASSEMBLER__

#define SPI_SET_SPEED_F_2		do {  SPI_CTRL_REGISTER = (1<<SPE) | (1<<MSTR) | (0<<SPR1) | (0<<SPR0); SPI_STATUS_REGISTER = (1<<SPI2X); } while(0)
#define SPI_SET_SPEED_F_4		do {  SPI_CTRL_REGISTER = (1<<SPE) | (1<<MSTR) | (0<<SPR1) | (0<<SPR0); SPI_STATUS_REGISTER = (0<<SPI2X); } while(0)
#define SPI_SET_SPEED_F_8		do {  SPI_CTRL_REGISTER = (1<<SPE) | (1<<MSTR) | (0<<SPR1) | (1<<SPR0); SPI_STATUS_REGISTER = (1<<SPI2X); } while(0)
#define SPI_SET_SPEED_F_16		do {  SPI_CTRL_REGISTER = (1<<SPE) | (1<<MSTR) | (0<<SPR1) | (1<<SPR0); SPI_STATUS_REGISTER = (0<<SPI2X); } while(0)
#define SPI_SET_SPEED_F_32		do {  SPI_CTRL_REGISTER = (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (0<<SPR0); SPI_STATUS_REGISTER = (1<<SPI2X); } while(0)
#define SPI_SET_SPEED_F_64		do {  SPI_CTRL_REGISTER = (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (0<<SPR0); SPI_STATUS_REGISTER = (0<<SPI2X); } while(0)
#define SPI_SET_SPEED_F_128		do {  SPI_CTRL_REGISTER = (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (1<<SPR0); SPI_STATUS_REGISTER = (0<<SPI2X); } while(0)

#if SPI_PRESCALER == 2
 #define SPI_SET_SPEED_FASTEST	 	SPI_SET_SPEED_F_2
#elif SPI_PRESCALER == 4
 #define SPI_SET_SPEED_FASTEST	 	SPI_SET_SPEED_F_4
#elif SPI_PRESCALER == 8
 #define SPI_SET_SPEED_FASTEST	 	SPI_SET_SPEED_F_8
#elif SPI_PRESCALER == 16
 #define SPI_SET_SPEED_FASTEST	 	SPI_SET_SPEED_F_16
#elif SPI_PRESCALER == 32
 #define SPI_SET_SPEED_FASTEST	 	SPI_SET_SPEED_F_32
#elif SPI_PRESCALER == 64
 #define SPI_SET_SPEED_FASTEST	 	SPI_SET_SPEED_F_64
#elif SPI_PRESCALER == 128
 #define SPI_SET_SPEED_FASTEST	 	SPI_SET_SPEED_F_128
#else
 #error "SPI Prescaler not defined correctly in mmc_spi.h."
#endif

#define SPI_SET_SPEED_SLOWEST		SPI_SET_SPEED_F_128

// security checks !
#if defined (STANDARD_SPI_WRITE) && defined (FAST_SPI_WRITE)
 #error "Define STANDARD_SPI_WRITE or FAST_SPI_WRITE only in mmc_spi.h, NOT both !"
#endif

#if defined (STANDARD_SPI_WRITE) && defined (HYPER_SPI_WRITE)
 #error "Define STANDARD_SPI_WRITE or HYPER_SPI_WRITE only in mmc_spi.h, NOT both !"
#endif

#if defined (FAST_SPI_WRITE) && defined (HYPER_SPI_WRITE)
 #error "Define FAST_SPI_WRITE or HYPER_SPI_WRITE only in mmc_spi.h, NOT both !"
#endif

#if !defined (STANDARD_SPI_WRITE) && !defined (FAST_SPI_WRITE) && !defined (HYPER_SPI_WRITE)
 #error "Define at least STANDARD_SPI_WRITE, FAST_SPI_WRITE or HYPER_SPI_WRITE in mmc_spi.h !"
#endif

#if defined (STANDARD_SPI_READ) && defined (FAST_SPI_READ)
 #error "Define STANDARD_SPI_READ or FAST_SPI_READ only in mmc_spi.h, NOT both !"
#endif

#if defined (STANDARD_SPI_READ) && defined (HYPER_SPI_READ)
 #error "Define STANDARD_SPI_READ or HYPER_SPI_READ only in mmc_spi.h, NOT both !"
#endif

#if defined (FAST_SPI_READ) && defined (HYPER_SPI_READ)
 #error "Define FAST_SPI_READ or HYPER_SPI_READ only in mmc_spi.h, NOT both !"
#endif

#if !defined (STANDARD_SPI_READ) && !defined (FAST_SPI_READ) && !defined (HYPER_SPI_READ)
 #error "Define at least STANDARD_SPI_READ, FAST_SPI_READ or HYPER_SPI_READ in mmc_spi.h !"
#endif

#if defined (USE_SPI0) && defined (USE_SPI1)
 #error "Define USE_SPI0 or USE_SPI1 only in mmc_spi.h, NOT both !"
#endif

#if !defined (USE_SPI0) && !defined (USE_SPI1)
 #error "Define at least USE_SPI0 or USE_SPI1 in mmc_spi.h !"
#endif

#if defined (HYPER_SPI_READ) || defined (HYPER_SPI_WRITE)
 #if SPI_PRESCALER != 2
  #error "SPI Prescaler has to be 2 for Hyper-SPI in mmc_spi.h !"
 #endif
#endif

#ifdef MMC_DEBUG_IDENTIFY
 #define DEBUG_MMC_IDENTIFY	printf  // Output via printf
#else
 #define DEBUG_MMC_IDENTIFY(...)	// No output
#endif

#ifdef MMC_DEBUG_CMD0_TIMEOUT
 #define DEBUG_MMC_CMD0_TIMEOUT	printf
#else
 #define DEBUG_MMC_CMD0_TIMEOUT(...)
#endif

#ifdef MMC_DEBUG_CMD1_TIMEOUT
 #define DEBUG_MMC_CMD1_TIMEOUT	printf
#else
 #define DEBUG_MMC_CMD1_TIMEOUT(...)
#endif

#ifdef MMC_DEBUG_COMMANDS
 #define DEBUG_MMC_COMMANDS printf
#else
 #define DEBUG_MMC_COMMANDS(...)
#endif

#ifdef MMC_DEBUG_SECTORS_READ
 #define DEBUG_MMC_SECTORS_READ	printf
#else
 #define DEBUG_MMC_SECTORS_READ(...)
#endif

#ifdef MMC_DEBUG_SECTORS_WRITE
 #define DEBUG_MMC_SECTORS_WRITE	printf
#else
 #define DEBUG_MMC_SECTORS_WRITE(...)
#endif

#endif
//@}
