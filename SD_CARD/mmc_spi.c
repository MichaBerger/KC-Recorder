/*! \file mmc_spi.c \brief MMC/SD-Functions */
//###########################################################
///	\defgroup MMC MMC/SD-Functions (mmc_spi.c)
///	\ingroup MMC
///	\code #include "dos.h" \endcode
///	\code #include "mmc_spi.h" \endcode
///	\par Uebersicht
//###########################################################
// File: mmc_spi.c
//
// Read-/Writeroutines for MMC MultiMedia cards and
// SD SecureDigital cards in SPI mode.
//
// This will work only for cards with 512 bytes block length !
//
// Note: Keep your cable as short as possible (5 cm /2 inch)
//       Don't use resistors as 5V->3V Levelshifters.
//       Don't use diodes as a voltage regulator.
//
//
// 04.07.2007 2GB card works 
//
// 28.06.2007 Started Support for 2GB and non SDHC 4GB cards
//
// 26.04.2007 Toshiba 4GB SDHC works !
//     
// 01.04.2007 Started SDHC support.
//
// 04.09.2006 Made new MMC_IO_Init(). Removed IO pin settings from
//            MMCIdentify(). Why ? See comments above MMC_IO_Init(). 
//
// 20.06.2006 Store value for SPI speed and switch to double speed
//            for Read/WriteSector(). Restore SPI speed at end of
//            routines.
//
// 29.09.2004 split SPI_WRITE() into SPI_WRITE() and SPI_WAIT()
//            speeds up program because CPU can do something else
//            while SPI hardware module is shifting in/out data 
//            see MMCReadSector() and MMCWriteSector()
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 20.10.2008
//#########################################################################
// hk@holger-klabunde.de
// http://www.holger-klabunde.de/index.html
//#########################################################################
// Compiler: WinAVR 4.1.1
//#########################################################################
//@{

 #include <avr/io.h>
 #include <avr/interrupt.h>

#include "dos.h"

#ifdef MMC_CARD_SPI
#include "mmc_spi.h"

#if defined (MMC_DEBUG_MAX_BUSY_WRITE) || defined (MMC_DEBUG_IDENTIFY) || defined (MMC_DEBUG_SECTORS_READ) || defined (MMC_DEBUG_SECTORS_WRITE) || defined (MMC_DEBUG_COMMANDS) || defined (MMC_DEBUG_CMD1_TIMEOUT) || defined (MMC_DEBUG_CMD0_TIMEOUT)
 #include "serial.h" // For debug only
 #include "printf.h" // For debug only
#endif

#ifdef MMC_DEBUG_MAX_BUSY_WRITE
 volatile U16 countbusy; // you have to count this in a timer ISR
 U16 maxbusytime,maxbusyhits;
#endif

unsigned char card_capacity; // Standard for MMC/SD or High for SDHC
U32 maxsect;                 // last sector on drive

#ifdef BUSY_BEFORE_CMD
 U8 card_type;
#endif

//######################################################
/*!\brief Send commands to MMC/SD
 * \param		command	Command code
 * \param		adress	Sector adress
 * \return 		Response of MMC/SD
 *
 */
U8 MMCCommand(U8 command, U32 adress)
//######################################################
{
#ifdef BUSY_BEFORE_CMD
 U16 timeout;
#else
 U8 timeout;
#endif

 U8 by;

// MMC_CS_ON(); // Write the dummy byte with CS high. My cards don't need it.

 SPI_WRITE(DUMMY_WRITE); // This dummy write is necessary for most SD cards !
 SPI_WAIT();

 MMC_CS_OFF();

#ifdef BUSY_BEFORE_CMD
 // Some SD Cards don't like this :( AARG, writing MMC was so fast :::(
 // This is the busy check after writing a sector !
 // Some SD cards send 0x00 if not being initialised before, so check card type
 if(card_type == MMC_CARD)
  {
   DEBUG_MMC_COMMANDS("Busy");

   timeout=0xFFFF;  //min. 65ms at 8MHz SPI, this should be enough time !
   do // a busy check
    {
     SPI_WRITE(DUMMY_WRITE);
     timeout--; // dec timeout while SPI module shifts out
     SPI_WAIT();
     by=SPI_DATA_REGISTER;

     DEBUG_MMC_COMMANDS(" 0x%02X",(U16)by);

     if(timeout == 0)
      {
       MMC_CS_ON();
       return 0xFF;
      }

    }while(by != 0xFF); // wait til busy has gone

     DEBUG_MMC_COMMANDS("\n");
  }
#endif //#ifdef BUSY_BEFORE_CMD

 SPI_WRITE(command);
 SPI_WAIT();
 SPI_WRITE((unsigned char)((adress & 0xFF000000)>>24)); // MSB of adress
 SPI_WAIT();
 SPI_WRITE((unsigned char)((adress & 0x00FF0000)>>16));
 SPI_WAIT();
 SPI_WRITE((unsigned char)((adress & 0x0000FF00)>>8));
 SPI_WAIT();
 SPI_WRITE((unsigned char)(adress & 0x000000FF));       // LSB of adress
 SPI_WAIT();
 SPI_WRITE(0x95); // Checksum for CMD0 GO_IDLE_STATE and dummy checksum for other commands
 SPI_WAIT();                             

 timeout=255;
 
 DEBUG_MMC_COMMANDS("Cmd %02u:",(U16)command-0x40);

 //wait for response
 do
  {
   SPI_WRITE(DUMMY_WRITE);
   SPI_WAIT();
   by=SPI_DATA_REGISTER;
   
   DEBUG_MMC_COMMANDS(" 0x%02X",(U16)by);

   timeout--;
   if(timeout==0) break; // no response
  } while(by==DUMMY_WRITE);

   DEBUG_MMC_COMMANDS("\n");

 return by;      			 
}

//######################################################
/*!\brief Read a sector from MMC/SD
 * \param		sector	Actual sector number
 * \param		buf	Buffer for data
 * \return 		0 if successfull
 */
U8 MMCReadSector(U32 sector, U8 *buf)
//######################################################
{
 U16 i;
 U8 by;
 U8 tmpSPSR;
 U8 tmpSPCR;

 DEBUG_MMC_SECTORS_READ("RS %lu 0x%08lX\n",sector,sector);

#ifdef CHECK_MAXSECTOR
 if(sector>=maxsect) return 1; //sectornumber too big
#endif

 tmpSPCR = SPI_CTRL_REGISTER;
 tmpSPSR = SPI_STATUS_REGISTER; //save old value

 SPI_SET_SPEED_FASTEST;

// MMC_CS_OFF(); // moved to MMCCommand()
// sector is the blockadress for SDHC

// if(card_capacity==STANDARD_CAPACITY) sector *= BYTE_PER_SEC; // SD/MMC This will work only up to 4GB
 if(card_capacity==STANDARD_CAPACITY) sector <<= 9; // SD/MMC This will work only up to 4GB
  
 by=MMCCommand(MMC_READ_BLOCK,sector);

 while(by!=START_BLOCK_TOKEN)
  {
   SPI_WRITE(DUMMY_WRITE);
   SPI_WAIT();
   by=SPI_DATA_REGISTER; // wait for card response

   DEBUG_MMC_SECTORS_READ("0x%02X ", (int)by);

#ifdef MMC_DEBUG_COMMANDS
// no way to come out of this :( skip this sector !?
     if(by==0x01) // ERROR !
      {
       // One of my SD cards sends this error. My cardreader has also
       // problems to read (NOT write !) the card completely. Trash. 
       DEBUG_MMC_COMMANDS("\nRead error 0x01 at sector %lu !\n",sector);

       MMC_CS_ON();

       SPI_CTRL_REGISTER = tmpSPCR;    //restore old value
       SPI_STATUS_REGISTER = tmpSPSR;    //restore old value

       // data buffer is not valid at this point !
       return 1;
      }
#endif      

  }//while(by!=START_BLOCK_TOKEN)

 DEBUG_MMC_SECTORS_READ("\n");

//----------------------------------------------------------------
#ifdef STANDARD_SPI_READ
  
 i=BYTE_PER_SEC;  // This routine is a little bit faster as for()                
 while(i)
  {
   SPI_WRITE(DUMMY_WRITE); // start shift in next byte
   i--;                    // dec loop counter while SPI shifts in
   SPI_WAIT();      // wait for next byte
   *buf++ = SPI_DATA_REGISTER;       // store byte in buffer
  }

 SPI_WRITE(DUMMY_WRITE); // shift in crc part1
 SPI_WAIT();
 SPI_WRITE(DUMMY_WRITE); // shift in crc part2
 SPI_WAIT();
#endif // STANDARD_SPI_READ
//----------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef FAST_SPI_READ
// The following code looks very strange !
// The idea is not to stop the cpu while SPI module transfers data.
// You have some cpu cycles until transmission has finished !
// You can use this time to do something like storing your last data
// or get your next data out of memory, doing some loop overhead.....
// Don't wait for end of transmission until you have done something better ;)

 SPI_WRITE(DUMMY_WRITE); // shift in first byte
 SPI_WAIT();             // we have to wait for the first byte, but ONLY for the first byte
 by=SPI_DATA_REGISTER;   // get first byte, but store later !
 
 SPI_WRITE(DUMMY_WRITE); // start shift in next byte

// Diese Schleife läßt sich nicht mehr verbessern :(
// Die Zeit zwischen SPI_WRITE() und SPI_WAIT() wird optimal ausgenutzt.
// Selbst ein 8 Bit Schleifenzähler bringt nichts. Ganz im Gegenteil.
// Man könnte noch INTs verbieten, aber dann kann man die Zeit nicht mehr messen ;)
 i=BYTE_PER_SEC-1;                 

 while(i) //execute the loop while transmission is running in background
  {
   *buf++ = by;             // store last byte in buffer while SPI module shifts in new data
   i--;
   SPI_WAIT();             // wait for next byte
   by=SPI_DATA_REGISTER;   // get next byte, but store later !
   SPI_WRITE(DUMMY_WRITE); // start shift in next byte
   // do the loop overhead at this point while SPI module shifts in new data
//   i--; // Code wird kleiner, aber dauert länger
  } 

 // last SPI_WRITE(DUMMY_WRITE); is shifting in crc part1 at this point
 *buf++ = by;               // store last byte in buffer while SPI module shifts in crc part1
 SPI_WAIT();
 SPI_WRITE(DUMMY_WRITE); // shift in crc part2
 SPI_WAIT();

#endif //FAST_SPI_READ
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//################################################################
#ifdef HYPER_SPI_READ   // Use this with AVR/ATMEGA only !
                        // Hand optimized, dangerous routine !
                        // SPI clock has to be F_CPU/2.
                        // THIS IS FOR TESTING ONLY !
                        // Don't use for development !
                        // Next compiler version may give you a kick in the ass.

// This should be done in a real assembly routine or in inline assembly.
// Does anyone have some for me ?

 SPI_WRITE(DUMMY_WRITE); // shift in first byte
 SPI_WAIT();             // we have to wait for the first byte, but ONLY for the first byte
 by=SPI_DATA_REGISTER;   // get first byte, but store later !
 
 SPI_WRITE(DUMMY_WRITE); // start shift in next byte

 NOP;                  // don't change anything here !
 i=BYTE_PER_SEC-1;                 
 
 while(i) //execute the loop while transmission is running in background
  {
   *buf++ =by;             // store last byte in buffer while SPI module shifts in new data
   i--;                    // Dec loop counter while SPI shifts in
   NOP;                  // Don't try to remove even a single NOP here !
   NOP;
   NOP;
   NOP;
   NOP;
   NOP; //GCC 4.11 only
   NOP; //GCC 4.11 only
   cli();                  // We have to disable Interrupts here !
   SPI_WRITE(DUMMY_WRITE); // start shift in next byte
   by=SPI_DATA_REGISTER;   // get next byte, but store later
   sei();                  // Enable Interrupts
   // do the loop overhead at this point while SPI module shifts in new data
  } 

 // last SPI_WRITE(DUMMY_WRITE); is shifting in crc part1 at this point
 *buf++ =by;               // store last byte in buffer while SPI module shifts in crc part1
 SPI_WAIT();
 SPI_WRITE(DUMMY_WRITE); // shift in crc part2
 SPI_WAIT();

#endif //HYPER_SPI_READ
//################################################################

 MMC_CS_ON();

 SPI_CTRL_REGISTER = tmpSPCR;    //restore old value
 SPI_STATUS_REGISTER = tmpSPSR;    //restore old value

 return 0;
}

#ifdef DOS_WRITE
//######################################################
/*!\brief Write a sector to MMC/SD
 * \param		sector	Actual sector number
 * \param		buf	Buffer for data
 * \return 		0 if successfull
 */
U8 MMCWriteSector(U32 sector, U8 *buf)
//######################################################
{
 U16 i;
 U8 by;
 U8 tmpSPSR;
 U8 tmpSPCR;

 DEBUG_MMC_SECTORS_WRITE("WS %lu 0x%08X\n",sector,sector);

#ifdef CHECK_MAXSECTOR
 if(sector>=maxsect) return 1; //sectornumber too big
#endif
 
 tmpSPCR = SPI_CTRL_REGISTER; //we should save this also !
 tmpSPSR = SPI_STATUS_REGISTER; //save old value

 SPI_SET_SPEED_FASTEST;

// MMC_CS_OFF(); // moved to MMCCommand()
// sector is the blockadress for SDHC

// if(card_capacity==STANDARD_CAPACITY) sector *= BYTE_PER_SEC; // SD/MMC This will work only up to 4GB
 if(card_capacity==STANDARD_CAPACITY) sector <<= 9; // SD/MMC This will work only up to 4GB

 
 MMCCommand(MMC_WRITE_BLOCK,sector);

//----------------------------------------------------------------
#ifdef STANDARD_SPI_WRITE
 SPI_WRITE(START_BLOCK_TOKEN); // start block token for next sector
 i=BYTE_PER_SEC;
 SPI_WAIT();
                  
 while(i)
  {
   SPI_WRITE(*buf++); // shift out next byte
   i--;               // dec loop counter while SPI shifts out
   SPI_WAIT();        // wait for end of transmission
  }

#endif //STANDARD_SPI_WRITE
//----------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef FAST_SPI_WRITE
 SPI_WRITE(START_BLOCK_TOKEN); // start block token for next sector

 i=BYTE_PER_SEC;                 
 while(i) // execute the loop while transmission is running in background
  {
   // do the loop overhead at this point while SPI module shifts out new data
   by = *buf++;   // get next data from memory while SPI module shifts out new data
   i--;
   SPI_WAIT();    // wait for end of transmission
   SPI_WRITE(by); // start shift out next byte
  }

 SPI_WAIT();      // wait til last byte is written to MMC
#endif //FAST_SPI_WRITE
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//################################################################
#ifdef HYPER_SPI_WRITE  // Use this with AVR/ATMEGA only !
                        // Hand optimized, dangerous routine !
                        // SPI clock has to be F_CPU/2.
                        // THIS IS FOR TESTING ONLY !
                        // Don't use for development !
                        // Next compiler version may give you a kick in the ass.
                        
// This should be done in a real assembly routine or in inline assembly.
// Does anyone have some for me ?

 SPI_WRITE(START_BLOCK_TOKEN); // start block token for next sector
 SPI_WAIT();
                  
 i=BYTE_PER_SEC;                 
 while(i)
  {
   SPI_WRITE(*buf++); // Shift out next byte
   i--;               // Dec loop counter while SPI shifts out
   NOP;             // Don't check SPI busy flag, but simply wait !
   NOP;             // Loop overhead plus NOP's have to be min. 16 CPU cycles
   NOP;             // Don't try to remove even a single NOP here !
   NOP;
   NOP;
   NOP;
   NOP;
   NOP;
   NOP; //GCC 4.11 only
   NOP; //GCC 4.11 only
  }

 SPI_WAIT();        // wait for end of LAST transmission

#endif //HYPER_SPI_WRITE
//################################################################

 SPI_WRITE(DUMMY_WRITE); // 16 bit crc follows data
 SPI_WAIT();
 SPI_WRITE(DUMMY_WRITE);
 SPI_WAIT();

 SPI_WRITE(DUMMY_WRITE); // read response
 SPI_WAIT();
 by=SPI_DATA_REGISTER & 0x1F;

 if(by != 0x05)   // data block accepted ?
  {
#ifdef MMC_DEBUG_SECTORS_WRITE
       if(card_capacity==STANDARD_CAPACITY) DEBUG_MMC_SECTORS_WRITE("Write error at sector %lu !\n",sector>>9);
       else DEBUG_MMC_SECTORS_WRITE("Write error at sector %lu !\n",sector);
       DEBUG_MMC_SECTORS_WRITE("Response 0x%02X\n",(int)by);
#endif      

   MMC_CS_ON();

   SPI_CTRL_REGISTER = tmpSPCR;    //restore old value
   SPI_STATUS_REGISTER = tmpSPSR;    //restore old value

   return 1;
  }

//Note: Moving this to MMCCommand may be dangerous.
//      Don't know if the card needs this clock to finish programming.
//      This may cause problems if after last sector write no MMCCommand
//      is called. MMC cards like this, SD cards NOT !

#ifdef BUSY_BEFORE_CMD
 if(card_type == SD_CARD || card_type == SDHC_CARD)
  {
#endif

 DEBUG_MMC_SECTORS_WRITE("WS Busy Check\n");

   do // a busy check 
    {
     SPI_WRITE(DUMMY_WRITE);
     SPI_WAIT();
     by = SPI_DATA_REGISTER;

     DEBUG_MMC_SECTORS_WRITE("0x%02X ", (int)by);
 
    }while(by == 0x00); // wait til busy has gone

 DEBUG_MMC_SECTORS_WRITE("\n");

#ifdef BUSY_BEFORE_CMD
  }
#endif

 MMC_CS_ON();

 SPI_CTRL_REGISTER = tmpSPCR;    //restore old value
 SPI_STATUS_REGISTER = tmpSPSR;    //restore old value

 return 0;
}
#endif //DOS_WRITE

//######################################################
/*!\brief Initialise io pin settings for MMC/SD
 * \return 		Nothing
 *
 * Removed this from MMCIdentify().
 * Maybe some other devices are connected to SPI bus.
 * All chip select pins of these devices should have
 * high level before starting SPI transmissions !
 * So first call MMC_IO_Init(), after that for example
 * VS1001_IO_Init(), SPILCD_IO_Init() and AFTER that MMCIdentify().
*/
void MMC_IO_Init(void)
//######################################################
{
// MMC_CS_ON();
 sbi(MMC_CS_PORT,MMC_CS_BIT); //MMC_CS set 1
 sbi(MMC_CS_DDR,MMC_CS_BIT);  //MMC_CS output

 cbi(MMC_SCK_PORT,MMC_SCK_BIT); //SCK set 0
 sbi(MMC_SCK_DDR,MMC_SCK_BIT);  //SCK output

 cbi(MMC_MISO_DDR,MMC_MISO_BIT);  //MISO input

 cbi(MMC_MOSI_PORT,MMC_MOSI_BIT); //MOSI set 0
 sbi(MMC_MOSI_DDR,MMC_MOSI_BIT);  //MOSI output
}

//######################################################
/*!\brief Get most important informations from MMC/SD
 * \return 		Nothing til now
 *
 * This function enables SPI transfers and determines
 * size of MMC/SD Card 
 */
U8 MMCIdentify(void)
//######################################################
{
 U8 by;

#ifndef BUSY_BEFORE_CMD
 U8 card_type;
#endif

 U16 i;
 U16 c_size_mult;
 U32 c_size; // now has 22 bits
 U16 read_bl_len;
 U8 csd_version; 
 U8 response[16];

// Init SPI with a very slow transfer rate first !

// SPCR SPI Controlregister
// SPIE=0; //No SPI Interrupt
// SPE=1;  //SPI Enable
// DORD=0; //Send MSB first
// MSTR=1; //I am the master !
// CPOL=0; //SCK low if IDLE
// CPHA=0; //SPI Mode 0
// SPR1=1; //SPI Clock = f/128 = 125kHz @16MHz Clock
// SPR0=1; //or f/64 if SPI2X = 1 in SPSR register
// SPSR SPI Statusregister
// SPI2X=0; // No double speed

 SPI_SET_SPEED_SLOWEST;

 // give min 74 SPI clock pulses before sending commands
 i=10;
 do
  {
   SPI_WRITE(DUMMY_WRITE);
   SPI_WAIT();
   i--;
  }while(i);                                      
 
// MMC_CS_OFF(); // moved to MMCCommand()

 DEBUG_MMC_IDENTIFY("Send CMD0\n");

 // Some of my cards give 0x01 response only after a few retrys of CMD0
 for(i=0; i<CMD0_RETRYS; i++)
  {
   //send CMD0 for RESET
   by=MMCCommand(MMC_CMD0,0);
   if(by == IN_IDLE_STATE) break;
  }  

 DEBUG_MMC_CMD0_TIMEOUT("CMD0 retrys %u\n", (U16)i);

 if(by != IN_IDLE_STATE) { MMC_CS_ON(); return CMD0_TIMEOUT; } // no response from card

 DEBUG_MMC_IDENTIFY("Send CMD8\n");

 card_capacity = STANDARD_CAPACITY; // assume standard capacity
 card_type = UNKNOWN_CARD; 

// CMD8 has to be send for SDHC cards to expand ACMD41 functions
// Check pattern = 0x22 Bits 7..0 (calculated for crc=0x95 !)
// VHS Voltage supplied 2.7-3.6V 0b0001 Bits 11..8
 c_size = (unsigned long)0x00000122 ; // reuse c_size here ;)

 // SEND_IF_COND
 if((MMCCommand(SD_CMD8,c_size) & ILLEGAL_COMMAND) == 0 ) // SD card version 2.00
  {
   GetResponse(response,4); //geht four more bytes of response3

   card_type = SD_CARD; // MMC don't know CMD8  !

   if(response[2] != 0x01) // test voltage range failed
    {
     DEBUG_MMC_IDENTIFY("Test Voltage range failed 0x%02X\n",(U16)response[2]);
     MMC_CS_ON(); return TEST_VOLTAGE_ERROR;
    }

   if(response[3] != 0x22) // test check pattern failed
    {
     DEBUG_MMC_IDENTIFY("Test pattern failed 0x%02X\n",(U16)response[3]);
     MMC_CS_ON(); return TEST_PATTERN_ERROR;
    }

// begin: Think we don't need this !
   DEBUG_MMC_IDENTIFY("Send CMD58\n");
   MMCCommand(SD_CMD58,0); // READ_OCR
   GetResponse(response,4); //geht four more bytes of response3
// end: Think we don't need this !

    i = 0;
    while(1) // repeat ACMD41 until card is ready
     {
      DEBUG_MMC_IDENTIFY("Send CMD55 ");
      MMCCommand(SD_CMD55,0); // SEND_APP_CMD

      DEBUG_MMC_IDENTIFY("ACMD41\n");

      if( MMCCommand(SD_ACMD41,0x40000000) == 0 ) break; // send with HCS bit set
      if(i++ > 1024) { MMC_CS_ON(); return ACMD41_TIMEOUT; }  // no response from card
     }

   DEBUG_MMC_CMD1_TIMEOUT("ACMD41 retrys %u\n", (U16)i);

   DEBUG_MMC_IDENTIFY("Send CMD58\n");

   // check for high capacity card now
   by=MMCCommand(SD_CMD58,0); // READ OCR
   GetResponse(response,4); //geht four more bytes of response3

   if(response[0] & 0x40)
    {
     card_capacity=HIGH_CAPACITY; // high capacity card if bit 30 of OCR is set
     card_type = SDHC_CARD;
     DEBUG_MMC_IDENTIFY("High capacity card\n");
    }
  }
 else  // SD card V1.xx or MMC
  {
   DEBUG_MMC_IDENTIFY("CMD8 illegal command\n");

  // Note: CMD1 is not supported by all SD cards ?
  // Thin 1,4mm cards don't accept CMD1 before sending ACMD41
  // Try ACMD41 first here !
  DEBUG_MMC_IDENTIFY("Send CMD55 ");

 // SEND_APP_CMD
  if((MMCCommand(SD_CMD55,0) & ILLEGAL_COMMAND) == 0 ) // SD card V1.xx
   {
    card_type = SD_CARD; // MMC don't know CMD55  !

    i=0;
    while(1) // repeat ACMD41 until card is ready
     {
      DEBUG_MMC_IDENTIFY("ACMD41\n");
      if( MMCCommand(SD_ACMD41,0) == 0 ) break; 
      if(i++ >1024) { MMC_CS_ON(); return ACMD41_TIMEOUT; } // no response from card
      
      DEBUG_MMC_IDENTIFY("Send CMD55 ");
      MMCCommand(SD_CMD55,0); // Repeat SEND_APP_CMD
     }

    DEBUG_MMC_CMD1_TIMEOUT("ACMD41 retrys %u\n", (U16)i);

   }
  else // MMC card
   {
    DEBUG_MMC_IDENTIFY("CMD55 illegal command\n");

   card_type = MMC_CARD;
   // Repeat CMD1 til result=0

   i=0;
   while(1)
    {
     DEBUG_MMC_IDENTIFY("Send CMD1\n");
     if( MMCCommand(MMC_CMD1,0) == 0 ) break;
     if(i++ > 1024) { MMC_CS_ON(); return CMD1_TIMEOUT; } // no response from card
    }

    DEBUG_MMC_CMD1_TIMEOUT("CMD1 retrys %u\n", (U16)i);

   }// if((MMCCommand(SD_CMD55,0) & ILLEGAL_COMMAND) == 0 ) // SD card version 1.00
    
  }// if((MMCCommand(SD_CMD8,c_size) & ILLEGAL_COMMAND) == 0 ) // SD card version 2.00

// Read CID
// MMCCommand(MMC_READ_CID,0); // nothing really interesting here

 DEBUG_MMC_IDENTIFY("Read CSD\n");

// Read CSD Card Specific Data
 by=MMCCommand(MMC_READ_CSD,0);

 while(by!=START_BLOCK_TOKEN)
  {
   SPI_WRITE(DUMMY_WRITE);
   SPI_WAIT();
   by=SPI_DATA_REGISTER; // Wait for card response
  }
   
 GetResponse(response,16); //CSD has 128 bits -> 16 bytes

 SPI_WRITE(DUMMY_WRITE); // 16 bit crc follows data
 SPI_WAIT();
 SPI_WRITE(DUMMY_WRITE);
 SPI_WAIT();

 MMC_CS_ON();

 DEBUG_MMC_IDENTIFY("CSD done\n");
// Here comes the hard stuff !
// Calculate disk size and number of last sector
// that can be used on your mmc/sd card
// Be aware ! Bit 127 of CSD is shifted in FIRST.

  by= response[5] & 0x0F;
  read_bl_len=1;
  read_bl_len <<= by;

#ifdef MMC_DEBUG_IDENTIFY
  if(read_bl_len > 512)
   {
    DEBUG_MMC_IDENTIFY("read_bl_len is %u !\n",read_bl_len);

// MMC_SET_BLOCKLEN always fails on my 2GB memorex card.
// Response is 0xFF. But it does work without this.
// All my other cards work too WITHOUT it. So: FORGET IT !!
//
// Auf deutsch: MMC_SET_BLOCKLEN braucht niemand !
// Geht komplett ohne bei allen meinen Karten.
//
/*
    // Set blocklen to 512 bytes if >512 bytes
    by=MMCCommand(MMC_SET_BLOCKLEN,512);
    if(by != 0)
     {
      DEBUG_MMC_IDENTIFY("SetBlocklen failed ! Resp=0x%02X\n",(int)by);
      return SET_BLOCKLEN_ERROR; // Set blocklen failed
     }
*/
   }
#endif

 // CSDVERSION = 0, MMC and Version 1.0/2.0 SD Standard Capacity Cards
 // CSDVERSION = 1, SDHC and Version 2.0 SD Cards

 if(card_type == MMC_CARD)
  {
   csd_version = 0;
  } 
 else // SD, SDHC
  {
   csd_version = response[0] >> 6;
  }
  
#ifdef MMC_DEBUG_IDENTIFY
 by = response[0] >> 6;
 DEBUG_MMC_IDENTIFY("CSD_STRUCT %u\t\t",(U16)by);
 by = (response[0] >> 2) & 0x0F;
 DEBUG_MMC_IDENTIFY("SPEC_VERSION %u\n",(U16)by);

 if(card_type == MMC_CARD)
  {
   DEBUG_MMC_IDENTIFY("MMC card\n");
  }
 else // SD / SDHC card
  {
   if(csd_version==0) DEBUG_MMC_IDENTIFY("SD card\n");
   if(csd_version==1) DEBUG_MMC_IDENTIFY("SDHC card\n");
  } 
#endif

 if(csd_version==2 || csd_version==3)
  {
   DEBUG_MMC_IDENTIFY("Unknown card\n");
   return CSD_ERROR; // error
  }

 if(csd_version==0)
  {
   //c_size has 12 bits
   c_size = response[6] & 0x03; //bits 1..0
   c_size <<= 10;
   c_size += (U16)response[7]<<2;
   c_size += response[8]>>6;

   by = response[9] & 0x03;
   by <<= 1;
   by += response[10] >> 7;
 
   c_size_mult=1;
   c_size_mult<<=(2+by);

//   drive_size=(unsigned long)(c_size+1) * (unsigned long)c_size_mult * (unsigned long)read_bl_len;
//   maxsect= drive_size / BYTE_PER_SEC;

   // Calculate this CAREFULLY ! maxsect has to be <= 2^32
   maxsect = read_bl_len / BYTE_PER_SEC;
   maxsect *= (unsigned long)(c_size+1) * (unsigned long)c_size_mult;
  }
 else
// if(csd_version==1)
  {
   //c_size has 22 bits
   c_size = response[7] & 0x3F; // 6 lower bits from here
   c_size <<= 16;
   c_size += (U16)response[8]<<8;
   c_size += response[9];
   maxsect=1024; // c_size is a multiple of 512kB -> 1024 sectors
   maxsect *= (c_size+1);
   c_size_mult=0; // to remove compiler warnings only
  }
  
 DEBUG_MMC_IDENTIFY("c_size %lu\t\tc_size_mult %u\n",c_size, c_size_mult);
 DEBUG_MMC_IDENTIFY("DriveSize %lu kB\tmaxsect %lu\n",(maxsect>>1), maxsect);

// Switch to high speed SPI. SPI Speed can be defined via SPI_PRESCALER in mmc_spi.h 
 SPI_SET_SPEED_FASTEST; 

 DEBUG_MMC_IDENTIFY("mmc_init() ok\n");

 return MMC_OK;
}

//######################################################
void GetResponse(U8 *buf, U8 numbytes)
//######################################################
{
#ifdef MMC_DEBUG_IDENTIFY
 unsigned char by;
#endif

 unsigned char i;

 i = numbytes;

 do
  {
   SPI_WRITE(DUMMY_WRITE);
   SPI_WAIT();

#ifdef MMC_DEBUG_IDENTIFY
   by = SPI_DATA_REGISTER;
   *buf++ = by;
//   DEBUG_MMC_IDENTIFY(" 0x%02X",(U16)SPI_DATA_REGISTER); // don't read this twice if SPI has a FIFO !
   DEBUG_MMC_IDENTIFY(" 0x%02X",(U16)by);
#else
   *buf++ = SPI_DATA_REGISTER;
#endif

   i--;
  }while(i);

 DEBUG_MMC_IDENTIFY("\n");
}

#endif //MMC_CARD_SPI
//@}
