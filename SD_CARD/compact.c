//#########################################################################
// File: compact.c
//
// Read-/Writeroutines for CompactFlash in LBA mode.
// CHS mode not supported.
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 14.08.2008
//#########################################################################
// hk@holger-klabunde.de
// http://www.holger-klabunde.de/index.html
//#########################################################################
// Compiler: AVR-GCC 3.4.5
// Compiler: AVR-GCC 4.1.1
//#########################################################################
//@{
#include <avr/io.h>

#include "dos.h"

#ifdef CF_DEBUG
 #include "printf.h"
#endif

#ifdef COMPACTFLASH_CARD

U32 maxsect;           // last sector on drive

//######################################################
static inline void CFSetAdr(unsigned char adr)
//######################################################
{
 unsigned char by;

// Autsch! Das könnte Pullups abschalten. by=CF_ADR_PIN;  //Portpins lesen
 by=CF_ADR_PORT;  //Portpins lesen
 by&=0xF0;       //D0..D3 löschen
 by|=adr;        //adr reinodern
 CF_ADR_PORT=by; //und ausgeben
}

//######################################################
unsigned char CFReadSector(unsigned long lba, unsigned char *buf)
//######################################################
{
 unsigned int i;
 unsigned char by;

#ifdef CF_DEBUG
 printf("RS %lu\n",lba);
#endif

 if(lba>=maxsect) return 1; //sectornumber too big

 if(CFWaitReady()) return 1;
 
// CFWriteAdr(CF_FEATURES,0); //Brauch ich nicht
 CFWriteAdr(CF_SECCOUNT,1);   //read one sector

 //This will work only up to 128GB, 2^28 * 512
 by=(unsigned char)lba;
 CFWriteAdr(CF_LBA0,by);            //D7..0
 by=(unsigned char)(lba>>8);
 CFWriteAdr(CF_LBA1,by);            //D15..8
 by=(unsigned char)(lba>>16);
 CFWriteAdr(CF_LBA2,by);            //D23..16
 by=(unsigned char)(lba>>24);
 by&=0x0F;              //only four LSBs used
 by|=0xE0;              //LBA-Mode Drive0
 CFWriteAdr(CF_LBA3,by);          //D27..24
 CFWriteAdr(CF_STACOM,CF_READ_SEC);

 if(CFWaitDrq()) return 2;

 CF_DATA_DIR_IN();
 CFSetAdr(CF_IO);
 CF_CS_OFF();

 i=BYTE_PER_SEC;

/*
 do
  {
   CF_RD_OFF();
   NOP; //Schon bei 8MHz muß man hier etwas warten
        //Es könnte evtl. nötig sein hier noch mehr
        //nop einzufügen, um möglichst viele CF zu lesen
        //Bei mir reichten bisher 2 nop

#if F_CPU >=16000000 //>=16MHz processor clock
   NOP;
#endif
   
   *buf++ = CF_READ_DATA();
   CF_RD_ON();
   i--;
  }while(i); 
*/

 do
  {
   CF_RD_OFF();

   i--;
   *buf++ = CF_READ_DATA();

   CF_RD_ON();
  }while(i); 

 CF_CS_ON();

 return 0;
}

#ifdef DOS_WRITE
//######################################################
unsigned char CFWriteSector(unsigned long lba, unsigned char *buf)
//######################################################
{
 unsigned int i;
 unsigned char by;

#ifdef CF_DEBUG
 printf("WS %lu\n",lba);
#endif

 if(lba>=maxsect) return 1; //sectornumber too big
 if(CFWaitReady()) return 1;

// CFWriteAdr(CF_FEATURES,0);
 CFWriteAdr(CF_SECCOUNT,1);   //write one sector

 //This will work only up to 128GB, 2^28 * 512
 by=(unsigned char)lba;
 CFWriteAdr(CF_LBA0,by);            //D7..0
 by=(unsigned char)(lba>>8);
 CFWriteAdr(CF_LBA1,by);            //D15..8
 by=(unsigned char)(lba>>16);
 CFWriteAdr(CF_LBA2,by);            //D23..16
 by=(unsigned char)(lba>>24);
 by&=0x0F;              //only four LSBs used
 by|=0xE0;              //LBA-Mode Drive0
 CFWriteAdr(CF_LBA3,by);          //D27..24
 CFWriteAdr(CF_STACOM,CF_WRITE_SEC);

 if(CFWaitDrq()) return 2;

 CFSetAdr(CF_IO);
 CF_DATA_DIR_OUT();
 CF_CS_OFF();

 i=BYTE_PER_SEC;

/*
 do
  {
   CF_WRITE_DATA(*buf++);

   CF_WR_OFF();
   NOP;
   CF_WR_ON();

   i--;
  }while(i);
*/

 do
  {
   CF_WRITE_DATA(*buf++);

   CF_WR_OFF();
   i--;
   CF_WR_ON();

  }while(i);

 CF_CS_ON();

 CF_DATA_DIR_IN();
 return 0;
}
#endif //DOS_WRITE

//######################################################
unsigned char CFIdentify(void)
//######################################################
{
// unsigned long maxtracks;
// unsigned int heads,sectors_per_track;
 unsigned int i;
 union Convert *cv;

// Kurze Version wenn Adressen,CS,RD,WR alle auf einem Port
// CF_CONTROL_DDR=0xFF; 	//Alles Ausgänge
// CF_CONTROL_PORT=0xFF; 	

//Flexible Version. CS,RD,WR dürfen auf beliebigen Pins/Ports verteilt sein
 CF_ADR_DDR |= 0x0F; 	//D0..3 for Adress

 sbi(CF_CS_DDR,CF_CS_BIT);
 CF_CS_ON();
 sbi(CF_RD_DDR,CF_RD_BIT);
 CF_RD_ON();
 sbi(CF_WR_DDR,CF_WR_BIT);
 CF_WR_ON();

 if(CFWaitReady()) return 1;
 
// cf_features=0; //Brauch ich nicht
// cf_seccount=1;   //einen Sektor lesen
// cf_lba0=1;       //LBA ist hier eigentlich egal, also 1
// cf_lba1=0;
// cf_lba2=0;
// cf_lba3=0xE0;
 CFWriteAdr(CF_STACOM,CF_IDENTIFY);

 if(CFWaitDrq()) return 2;
  
 for(i=0; i<BYTE_PER_SEC; i++) iob[i]=CFReadAdr(CF_IO);
 
// cv=(union Convert *)&iob[2];
// maxtracks=cv->ui;          //Anzahl Tracks

// cv=(union Convert *)&iob[6];
// heads=cv->ui;              //Anzahl Heads

// cv=(union Convert *)&iob[12];
// sectors_per_track=cv->ui; //Sectors per Track

// this is the only information we need 
// Weder Little Endian noch Big Endian ?
 cv=(union Convert *)&iob[14];
 maxsect=(unsigned long)cv->ui << 16;         // number of sectors
 cv=(union Convert *)&iob[16];
 maxsect+=cv->ui;

 return 0;
}

//######################################################
unsigned char CFWaitReady(void)
//######################################################
{
 unsigned char by;

 by=CFReadAdr(CF_STACOM);
 if(by & 0x01) return 1; //Fehler !

 do
  {
   by=CFReadAdr(CF_STACOM);
   by&=0xF0;
   
  }while(by!=0x50); //Endlosschleife möglich !
  
 return 0; 
}

//######################################################
unsigned char CFWaitDrq(void)
//######################################################
{
 unsigned char by;

 by=CFReadAdr(CF_STACOM);
 if(by & 0x01) return 1; //Fehler !

 do
  {
   by=CFReadAdr(CF_STACOM);
   by&=0xF8;
   
  }while(by!=0x58); //Endlosschleife möglich !
  
 return 0; 
}

//######################################################
unsigned char CFReadAdr(unsigned char adr)
//######################################################
{
 unsigned char by;
 
 CF_DATA_DIR_IN();
 CFSetAdr(adr);
 CF_CS_OFF();
 CF_RD_OFF();
 NOP; //Schon bei 8MHz muß man hier etwas warten
 NOP; //Es könnte evtl. nötig sein hier noch mehr
        //nop einzufügen, um möglichst viele CF zu lesen
        //Bei mir reichten bisher 2 nop
        //Sicherheitshalber noch einen mehr
 NOP;
        //Sonst meldet CFWaitReady() immer Fehler

#if F_CPU >=16000000 //>=16MHz processor clock
   NOP;
#endif
   
 by=CF_READ_DATA();
 CF_RD_ON();
 CF_CS_ON();
 return by;
}

//######################################################
unsigned char CFRead(void)
//######################################################
{
 unsigned char by;
 
 CF_RD_OFF();
 NOP; //Schon bei 8MHz muß man hier etwas warten
 NOP; //Es könnte evtl. nötig sein hier noch mehr
        //nop einzufügen, um möglichst viele CF zu lesen
        //Bei mir reichten bisher 2 nop
        //Sicherheitshalber noch einen mehr
 NOP;

#if F_CPU >=16000000 //>=16MHz processor clock
   NOP;
#endif

 by=CF_READ_DATA();
 CF_RD_ON();
 return by;
}

//######################################################
void CFWriteAdr(unsigned char adr, unsigned char dat)
//######################################################
{
 CF_DATA_DIR_OUT();
 CFSetAdr(adr);
 CF_WRITE_DATA(dat);

 CF_CS_OFF();

 NOP;

 CF_WR_OFF();

 NOP;
#if F_CPU >=16000000 //>=16MHz processor clock
   NOP;
#endif

 CF_WR_ON();
 CF_CS_ON();
}

#endif //COMPACTFLASH_CARD
//@}
