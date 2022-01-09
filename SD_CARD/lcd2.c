//#########################################################################
// File: LCD.C
//
// Ansteuerung einer LCD-Anzeige im
// 8 Bit Daten Mode oder 4 Bit Daten Mode
//
// Controller:
// HD44780, KS066 oder KS073. Siehe lcd.h.
//
// Todo:
// Noch einiges :(
//
//#########################################################################
// Last change: 12.03.2008
//#########################################################################
// hk@holger-klabunde.de
// http://www.holger-klabunde.de/index.html
//#########################################################################
// Compiler: AVR-GCC 4.1.1
//#########################################################################
//@{
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "../main.h"

/*
#if !defined (F_CPU)
// #define F_CPU  1000000 // lieber langsam als gar nicht ;) Quark !!
 #define F_CPU  20000000 // je höher F_CPU desto länger die Schleifen ;)
# warning "F_CPU not defined in lcd.c ! Assume 20.000000 MHz."
#endif
*/

#include "lcd2.h"

void InitHD44780(void);
void InitKS073(void);
void InitKS066(void);
void InitKS070(void);

//*******************************************
// Set E high, wait a little bit and set E low
//*******************************************
void Clock_E(void)
{
 LCD_E_ON;
 Delay1us(1);
 LCD_E_OFF;	
 Delay1us(1);
}

//*****************************************
// Commando ans Display senden
//*****************************************
void LCDCommand(unsigned char command)
{
 LCD_RS_OFF; //Controlregister
 LCDWriteByte(command);
 LCD_RS_ON; //Zurück auf Datenregister
}

#ifdef INTERFACE_4_BIT
//*******************************************
// Schreibt ein Datennibble in die LCD-Anzeige
//*******************************************
void LCDWriteNibble(unsigned char data)
{

#ifdef INTERFACE_4_BIT_FIX_LOW
 unsigned char c;
 
 c = LCD_DATA_PORT;
 c &= 0xF0;
 c |= data;
 LCD_DATA_PORT = c;
#endif

#ifdef INTERFACE_4_BIT_FIX_HIGH
 unsigned char c;
 
 c = LCD_DATA_PORT;
 c &= 0x0F;
 c |= data<<4;
 LCD_DATA_PORT = c;
#endif

#ifdef INTERFACE_4_BIT_FIX_MID
 unsigned char c;
 
 c = LCD_DATA_PORT;
 c &= 0xC3;
 c |= data<<2;
 LCD_DATA_PORT = c;
#endif

#ifdef INTERFACE_4_BIT_MIX
 if(data & 0x01) { sbi(LCD_D4_PORT, LCD_D4_BIT); }
 else { cbi(LCD_D4_PORT, LCD_D4_BIT); }

 if(data & 0x02) { sbi(LCD_D5_PORT, LCD_D5_BIT); }
 else { cbi(LCD_D5_PORT, LCD_D5_BIT); }

 if(data & 0x04) { sbi(LCD_D6_PORT, LCD_D6_BIT); }
 else { cbi(LCD_D6_PORT, LCD_D6_BIT); }

 if(data & 0x08) { sbi(LCD_D7_PORT, LCD_D7_BIT); }
 else { cbi(LCD_D7_PORT, LCD_D7_BIT); }
#endif

 Clock_E();	
}
#endif

//*******************************************
// Schreibt ein Datenbyte in die LCD-Anzeige
//*******************************************
void LCDWriteByte(unsigned char data)
{

#ifdef INTERFACE_4_BIT
 LCDWriteNibble(data >> 4);
 LCDWriteNibble(data & 0x0F);
#endif
 
#ifdef INTERFACE_8_BIT
 LCD_DATA_PORT=data;
 Clock_E();	
#endif

#ifdef LCD_BUSY_CHECK
#else
 Delay1us(50); //etwas auf die Anzeige warten
#endif
}

//######################################################
//Schreibt einen Text aus dem RAM in die LCD-Anzeige
//######################################################
void LCDWrite(char *s)
{
 char c;
 
  while((c = *s++))
   {
    c=ConvertChar(c);
    if(c) LCDWriteByte(c);
   }
}

//######################################################
// Schreibt einen Text aus dem FLASH in die LCD-Anzeige
// Aufruf LCDWrite_P("text") ! Siehe lcd.h.
//######################################################
void LCDWrite_p(const char *s)
{
 char c;
 
  while((c=pgm_read_byte(s++)))
   {
    c=ConvertChar(c);
    if(c) LCDWriteByte(c);
   }
}

//######################################################
// Übersetzungstabelle für Sonderzeichen
//######################################################
char ConvertChar(char data)
{
 char ps;

 ps = data;
 
#ifdef EADIP204
  //Übersetzungstabelle Win zu EADIP204 Sonderzeichen
 switch(ps)
  {
   case 'ä' : ps=0x7B; break; //Umlenkung auf neuen Zeichen Code
   case 'ö' : ps=0x7C; break;
   case 'ü' : ps=0x7E; break;
   case 'ß' : ps=0xBE; break;
   case 'Ä' : ps=0x5B; break;
   case 'Ö' : ps=0x5C; break;
   case 'Ü' : ps=0x5E; break;
   case '°' : ps=0x80; break;
   case 'µ' : ps=0x8F; break;
   case '_' : ps=0xC4; break;
   case '@' : ps=0xA0; break;
   case '[' : ps=0xFA; break;
   case ']' : ps=0xFC; break;
   case '|' : ps=0xFE; break;
   case '{' : ps=0xFD; break;
   case '}' : ps=0xFF; break;
   case '~' : ps=0xCE; break;
   case '§' : ps=0x5F; break;
   case '$' : ps=0xA2; break;
   case '^' : ps=0x1D; break;
   case '\\' : ps=0xFB; break;
   case 0x0A : ps=0; break;  //LF
   case 0x0D : ps=0; break;  //CR 
  }
#elif defined CUSTOM_TABLE
  //Übersetzungstabelle Sonderzeichen selbst definiert
 switch(ps)
  {
   case 0x0A : ps=0; break;  //LF
   case 0x0D : ps=0; break;  //CR 
  }

#else
  //Standard Übersetzungstabelle Win zu LCD Sonderzeichen
 switch(ps)
  {
   case 'ä' : ps=0xE1; break; //Umlenkung auf neuen Zeichen Code
   case 'ö' : ps=0xEF; break;
   case 'ü' : ps=0xF5; break;
   case 'ß' : ps=0xE2; break;
   case '°' : ps=0xDF; break;
   case 'µ' : ps=0xE4; break;
   case 0x0A : ps=0; break;  //LF
   case 0x0D : ps=0; break;  //CR 
//   case 'Ä' : ps=0x00; break; //Umlenkung ins Character RAM
//   case 'Ö' : ps=0x01; break;
//   case 'Ü' : ps=0x02; break;
  }

#endif //#ifdef EADIP

 return ps;
}

//######################################################
// Springt auf eine  bestimmte Zeile und Spalte
// der LCD-Anzeige. Links oben ist Position 1,1
// Gültig für bis zu 4 Zeilen
//######################################################
void LCDPos(unsigned char zeile,unsigned char spalte)
{
 unsigned char buff;

 // Bei ungültigen Werten tue einfach nichts
 if(zeile == 0 || zeile > LCD_LINES) return;
 if(spalte == 0 || spalte > LCD_ROWS) return;

#if defined (HD44780) || defined (KS066) || defined (KS070)
// 0x80 wegen Display Adress Set.
// Offset's bei HD44780
// Zeile 1 -> 0x80+0x00 = 0x80
// Zeile 2 -> 0x80+0x40 = 0xC0 
// Zeile 3 -> 0x80+0x00+0x14 = 0x94 
// Zeile 4 -> 0x80+0x40+0x14 = 0xD4 

 switch(zeile)
  {
   case 1: buff = 0x80 -1 + spalte; break;
#if LCD_LINES >1
   case 2: buff = 0xC0 -1 + spalte; break;
#endif
#if LCD_LINES >2
   case 3: buff = 0x94 -1 + spalte; break;
#endif
#if LCD_LINES >3
   case 4: buff = 0xD4 -1 + spalte; break;
#endif
   default : buff = 0x80; break; //1,1 remove compiler warning only
  }
#endif //#ifdef HD44780

#ifdef KS073
// Offset's anders als bei HD44780 Displays !
// 2 Lines
// Zeile 1 -> 0x80+0x00 = 0x80
// Zeile 2 -> 0x80+0x40 = 0xC0 

// 4 Lines
// Zeile 1 -> 0x80+0x00 = 0x80
// Zeile 2 -> 0x80+0x20 = 0xA0 
// Zeile 3 -> 0x80+0x40 = 0xC0 
// Zeile 4 -> 0x80+0x60 = 0xE0 

 switch(zeile)
  {
   case 1: buff = 0x80 -1 + spalte; break;
#if LCD_LINES ==2
   case 2: buff = 0xC0 -1 + spalte; break;
#endif

#if LCD_LINES >2
   case 2: buff = 0xA0 -1 + spalte; break;
   case 3: buff = 0xC0 -1 + spalte; break;
#endif
#if LCD_LINES >3
   case 4: buff = 0xE0 -1 + spalte; break;
#endif
   default : buff = 0x80; break; //1,1 remove compiler warning only
  }
#endif // #ifdef KS073

 LCDCommand(buff); 
 Delay1us(50); //etwas auf die Anzeige warten
}

//######################################################
// Löscht die LCD Anzeige
// Cursor steht dann auf Position 1,Zeile 1
// Gilt auch für mehrzeilige Anzeigen
//######################################################
void LCDCls(void)
{
 LCDCommand(0x01); //CLS
 Delay1ms(5);
}

//######################################################
//Stellt die Funktionsweise der LCD Anzeige ein
//######################################################
void LCDInit(void)
{

 sbi(LCD_RS_DDR,LCD_RS_BIT); //LCD_RS is output
 sbi(LCD_E_DDR,LCD_E_BIT); //LCD_E is output
// sbi(LCD_RW_DDR,LCD_RW_BIT); //LCD_RW is output
 LCD_RS_OFF;  //Control Register
 LCD_E_OFF;  //LCD-Anzeige abwählen
// LCD_RW_OFF;  //LCD schreiben

#ifdef INTERFACE_4_BIT_FIX_LOW
 LCD_DATA_DDR |= 0x0F;
#endif

#ifdef INTERFACE_4_BIT_FIX_HIGH
 LCD_DATA_DDR |= 0xF0;
#endif

#ifdef INTERFACE_4_BIT_FIX_MID
 LCD_DATA_DDR |= 0x3C;
#endif

#ifdef INTERFACE_4_BIT_MIX
 sbi(LCD_D4_DDR, LCD_D4_BIT); // Set pins to outputs
 sbi(LCD_D5_DDR, LCD_D5_BIT);
 sbi(LCD_D6_DDR, LCD_D6_BIT);
 sbi(LCD_D7_DDR, LCD_D7_BIT);
#endif

#ifdef INTERFACE_8_BIT
 LCD_DATA_DDR  = 0xFF;   //All outputs D7..0
 LCD_DATA_PORT = 0x00;  //LCD Data Port all zero
#endif

#ifdef INTERFACE_8_BIT_MIX
 // You really don't want this ;)
#endif

 Delay1ms(255); //Möglichen Power On Reset abwarten

#ifdef HD44780
 InitHD44780();
#endif

#ifdef KS066
 InitKS066();
#endif

#ifdef KS070
 InitKS070();
#endif

#ifdef KS073
 InitKS073();
#endif

 LCDCommand(0x08); 	//Display off
 LCDCls();
 LCDCommand(0x06); 	//Increment, Display Freeze

// LCDCommand(0x0F); 	//Display on, Cursor on ,Cursor blink
// LCDCommand(0x0E); 	//Display on, Cursor on ,Cursor no blink
 LCDCommand(0x0C); 	//Display on, Cursor off ,Cursor no blink
 LCDCommand(0x14); 	//Cursor Move,Shift off,Right Shift
}

//#######################################################
// Initialisation for HD44780
//#######################################################
#ifdef HD44780
void InitHD44780(void)
{
 unsigned char i;

//Software Reset laut Hitachi Datenblatt
 for(i=0; i<3; i++)
  {
#ifdef INTERFACE_8_BIT
   LCDWriteByte(0x30);    //8 Bit Operation
#endif

#ifdef INTERFACE_4_BIT
   LCDWriteNibble(0x03);
#endif

   Delay1ms(100); //Ein bißchen warten auf die Anzeige
  }

#ifdef INTERFACE_8_BIT
#if LCD_LINES >1
 LCDWriteByte(0x38);    //8 Bit Operation,2 Line,5x7 Font
#else
 LCDWriteByte(0x30);    //8 Bit Operation,1 Line,5x7 Font
#endif
 Delay1ms(5);
#endif //#ifdef INTERFACE_8_BIT

#ifdef INTERFACE_4_BIT
 LCDWriteNibble(0x02);
 Delay1ms(5);
 #if LCD_LINES >1
  LCDWriteByte(0x28);    //4 Bit Operation,2 Line,5x7 Font
 #else
  LCDWriteByte(0x20);    //4 Bit Operation,1 Line,5x7 Font
 #endif
 Delay1ms(5);
#endif //#ifdef INTERFACE_4_BIT
}
#endif //#ifdef HD44780

//#######################################################
// Initialisation for KS073
//#######################################################
#ifdef KS073
void InitKS073(void)
{
#ifdef INTERFACE_4_BIT
 LCDWriteNibble(0x02);  //4 Bit Operation
 Delay1ms(5);
 LCDWriteByte(0x2C);    //4 Bit Operation, RE=1
#endif

#ifdef INTERFACE_8_BIT
 LCDWriteByte(0x30);  //8 Bit Operation
 Delay1ms(5);
 LCDWriteByte(0x3C);   //8 Bit Operation, RE=1
#endif
 Delay1ms(5);

#if LCD_LINES >2
 LCDWriteByte(0x09);    //5 Font, 4 Line
#else
 LCDWriteByte(0x08);    //5 Font, 1 or 2 Line
#endif
 Delay1ms(5);

#ifdef INTERFACE_8_BIT
 LCDWriteByte(0x38);    //8 Bit Operation, RE=0
#endif

#ifdef INTERFACE_4_BIT
 LCDWriteByte(0x28);    //4 Bit Operation, RE=0
#endif
 Delay1ms(5);
}
#endif //#ifdef KS073

//#######################################################
// Initialisation for KS066
//#######################################################
#ifdef KS066
void InitKS066(void)
{
#ifdef INTERFACE_4_BIT
 LCDWriteNibble(0x02);  //4 Bit Operation
 LCDWriteNibble(0x02);  //4 Bit Operation
 #if LCD_LINES >1
  LCDWriteNibble(0x0C);   //2 Lines, display on
 #else
  LCDWriteNibble(0x04);   //1 Line, display on
 #endif
#endif

#ifdef INTERFACE_8_BIT
 #if LCD_LINES >1
  LCDWriteByte(0x3C);    //2 Lines, display on
 #else
  LCDWriteByte(0x34);    //1 Line, display on
 #endif
#endif
 Delay1ms(5);

}
#endif //#ifdef KS066

//#######################################################
// Initialisation for KS070
//#######################################################
#ifdef KS070
void InitKS070(void)
{
#ifdef INTERFACE_4_BIT
 LCDWriteNibble(0x02);  //4 Bit Operation
 LCDWriteNibble(0x02);  //4 Bit Operation
 #if LCD_LINES >1
  LCDWriteNibble(0x08);   //2 Lines
 #else
  LCDWriteNibble(0x00);   //1 Line
 #endif
#endif

#ifdef INTERFACE_8_BIT
 #if LCD_LINES >1
  LCDWriteByte(0x38);    //2 Lines
 #else
  LCDWriteByte(0x30);    //1 Line
 #endif
#endif
 Delay1ms(5);

}
#endif //#ifdef KS070

//#######################################################
// Setzt ein neues Zeichen (0-7) im CGRAM.
// "buf" ist das Array mit den 8 Datenbytes.
void LCDSetCGRAM(unsigned char adr, unsigned char *buf)
//#######################################################
{
 unsigned char offset;
 
 if(adr>7) return;
 
 offset = adr<<3; //adr * 8;
 offset += 0x40;  //CGRam Flag setzen
 LCDCommand(offset);

 for(offset=0; offset<8; offset++) //8 Codes pro Zeichen übertragen
  {
   LCDWriteByte(*buf++);
  }

 LCDCommand(0x80); //DD_RAM Adresse 0 setzen, besser ist das
}

//#######################################################
//Zeigt ein Byte im HexCode (ohne 0x vorne) an
void LCDHexChar(unsigned char by)
//#######################################################
{
 unsigned char buff;

 buff=by>>4; //Highnibble zuerst
 if(buff<10) buff+='0'; //ASCII Code erzeugen
 else buff+=0x37;        //Großbuchstaben
 LCDWriteByte(buff);

 buff=by&0x0f; //Danach das Lownibble
 if(buff<10) buff+='0'; //ASCII Code erzeugen
 else buff+=0x37;        //Großbuchstaben
 LCDWriteByte(buff);
}

//#######################################################
void LCDHexInt(unsigned int l)
//#######################################################
{
 LCDHexChar((unsigned char)(l>>8));
 LCDHexChar((unsigned char)(l));
}

//#######################################################
void LCDHexLong(unsigned long l)
//#######################################################
{
 LCDHexChar((unsigned char)(l>>24));
 LCDHexChar((unsigned char)(l>>16));
 LCDHexChar((unsigned char)(l>>8));
 LCDHexChar((unsigned char)(l));
}

//###################################################################################
//Nicht sonderlich genau !! Braucht aber util/delay.h nicht
void Delay1ms(unsigned int time)
//###################################################################################
{
 while(time--) Delay1us(1000);
}

//###################################################################################
//Nicht sonderlich genau !! Braucht aber util/delay.h nicht
void Delay1us(unsigned int time)
//###################################################################################
{
 while(time--)
  {
   //Ein CPU-Clock = 0.250us bei 4MHz
   //Ein CPU-Clock = 0.166us bei 6MHz
   //Ein CPU-Clock = 0.125us bei 8MHz
   //Ein CPU-Clock = 0.0904us bei 11.0592MHz
   //Ein CPU-Clock = 0.0625us bei 16MHz
   //Ein CPU-Clock = 0.0500us bei 20MHz

#if F_CPU >= 20000000
   NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP;
   NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP;
   NOP; NOP;
#elif F_CPU >= 16000000
   NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP;
   NOP; NOP; NOP; NOP; NOP; NOP;
#elif F_CPU >= 11059200
   NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP;
#elif F_CPU >= 8000000
    NOP; NOP; NOP; NOP; NOP; NOP;
#elif F_CPU >= 6000000
   NOP; NOP; NOP; NOP;
#elif F_CPU >= 4000000
   NOP; NOP;
#else
   NOP;
#endif

  }
}

//@}
