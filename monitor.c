#include "main.h"
#include "filework.h"
#include "RLE.h"
#include "SD_CARD/dos.h"

// local prototypes
void displayHelp(void);
uint16_t dumpRAM(uint16_t addr);
int8_t getArgs(void);
void clearLineBuffer(void);
uint8_t convertHexByte(uint8_t *here);
void record(void);
void KCgetFileName(void);
void KCrecord(void);
void KCreplay(void);
void prepareTAPheader(void);
void replay(void);		// play previously recorded data
void beep(uint16_t n);	// emit tuning signal
void playBlock(uint16_t pos);
void displayInitScreen(void);
void displayMenu(char * c);
uint8_t isBasic(uint8_t * there);
uint8_t waitEdge(void);

uint16_t currAddr = 0;	// default memory address for dump
uint16_t Arg[3];		// numeric arguments for dump etc.
int8_t	 nArg;			// number of arguments
uint8_t	 LineBuf[82];	// Line Buffer
uint8_t  LinePtr;		// Pointer for Line Buffer
uint8_t	 ArgPtr;		// from here on arguments etc
uint8_t prompt[] = ">";
uint8_t	cmd = 0;
uint8_t recMem[MEMSIZE];// buffer for header contents
uint32_t RAMcounter;			// record memory pointer
char filename[] = "             "; // string for file name
volatile uint8_t pending = 0;	// pending flag for Key handling
volatile uint8_t breakFlag = 0;	// flag for abandoning record or play
uint8_t isTAP = 0;			// offset in case a TAP file was loaded
uint8_t	RLEmode = FALSE;	// special mode for Run Length Encoding

ISR (PCINT3_vect)
{
uint8_t keyFlag;
	keyFlag = ~(DDRD | PIND) & KEYMASK;
	if ((keyFlag) && (pending == 0)) {			// action only if pin is input and level is 0
		if (keyFlag & (1<<REC_KEY)) {
			serbuffer[serInPointer++] = 'r';	// initiate RECORD action
			serbuffer[serInPointer++] = '\r';
			serInPointer &= SERBUFMASK;
			buf_usage +=2;
			pending = 1;
		}
		else {
			if (keyFlag & (1<<PLAY_KEY)) {
				serbuffer[serInPointer++] = 'p';	// initiate PLAY action
				serbuffer[serInPointer++] = '\r';
				serInPointer &= SERBUFMASK;
				buf_usage +=2;
				pending = 1;
			}
			else {
				if (keyFlag & (1<<DIR_KEY)) {
					serbuffer[serInPointer++] = 'f';	// initiate DIR action
					serbuffer[serInPointer++] = '\r';
					serInPointer &= SERBUFMASK;
					buf_usage +=2;
					pending = 1;
				}
			}
		}
	}
	if (keyFlag & (1<<BREAK_KEY)) breakFlag = 1;	// raise break flag
}	// end of PCINT3_vect

void monitor(void)
{
uint8_t c;
uint16_t i;
uint8_t dumpMode = 0;
	do {
		MMC_IO_Init();	// init SD Card interface
		if (GetDriveInformation() == F_OK) break;
		lcd_string_xy(2,1,"SD-Card Err!");
	} while (1);		// hold until a SD-Card was inserted...

	displayHelp();	// help screen over serial line
	displayInitScreen();	// info on LCD at power up

	do {	// start processing another line
		displayMenu("Rec Play End DIR");
		dispFileName(filename);	// display the file name in case there is a loaded file
		if (!dumpMode) {
			printf(CRLF);	// echo incoming CR as CRLF
			printf(prompt);
		}
		cmd = 0;	// no command found yet
		LinePtr = 0;
		while ((c = toupper(uart_getChar())) != '\r') {
			LineBuf[LinePtr++] = c;
			if (!cmd && (c > ' ')) {
				cmd = c;
				ArgPtr = LinePtr;
			}
			uart_putChar(c);	// echo input unless Intel Hex
		}	// now the line should be in the line buffer
		breakFlag = 0;	// clear pending break at new command
		if (cmd) dumpMode = 0;	// exit dump mode with any command
		switch (cmd) {
			case 'R':	// start Recording
				record();
				break;
			case 'P':	// replay previosuly recorded data
				replay();
				breakFlag = 0;	// reset Break flag in case it was used...
				break;
			case 'D':
				dumpMode = 1;		// set flag for dump mode
				nArg = getArgs();	// extract up to 3 Arguments 
				if (nArg == ERR) printf("?");
				else {
					sermem_reset();		// reset serial memory
					for (i=0;i<Arg[0];i++) sermem_readByte(); // fast forward to Addr
					printf(CRLF);
					currAddr = dumpRAM(Arg[0]);
				}
				break;
			case 'E': eraseRAM(); break;
			case 'F':
				FileDialog();
				pending = 0; 
				break;
			case '?': displayHelp(); break;
			case '.':
				dumpMode = 0;	// end hex dump mode
				break;
			case 0: 
				if (dumpMode) {	// continue with hex/ascii dump
					currAddr = dumpRAM(currAddr);
				}
				break;	// if only Enter was pressed
			default: printf("?"); break;
		}
	} while (1);
}	// end of monitor

void displayHelp(void)
{
	printf(CRLF "KC85 Data Recorder" CRLF);
	printf("Version " VERSION " by M.Berger" CRLF);

	printf("Commands Overview:" CRLF CRLF);
	printf(" D [addr]         Dump Memory Hex+ASCII" CRLF);
	printf(" .                Stop File Dump" CRLF);
	printf(" E                Erase record memory" CRLF);
	printf(" F                turn control over to device's File Dialogue" CRLF);
	printf(" P                Play" CRLF);
	printf(" R                Record" CRLF);
	printf(" ?                Display this overview" CRLF);
	printf(CRLF);
}	// end of displayHelp

void eraseRAM(void)
{
uint32_t i;
	for (i=0;i<MEMSIZE;i++) recMem[i] = 0x00;
	RAMcounter = 0;	// reset data pointer
	sermem_init();	// reset serial memory
	for (i=0;i<65536;i++) sermem_writeByte(0);	// write 0 to first 64K
	lcd_string_xy(0,1,"                ");	// erase displayed file name
	sermem_init();	// and reset mem pointer again!
}	// end of eraseRAM

uint8_t readRAMByte(uint16_t Addr)
{
	return recMem[Addr];
}	// end of readRAMByte

uint16_t dumpRAM(uint16_t Addr)
{
uint8_t  c;
uint16_t i;
	for (i=0;i<8;i++) {
		clearLineBuffer();
		Bin2Hex(LineBuf,Addr + i * BYTES_LINE,4);
		for (uint8_t j=0;j<BYTES_LINE;j++) {
			c = sermem_readByte();	// get byte from SERMEM
			Bin2Hex((LineBuf + 7 + 3 * j),(uint16_t) c,2);	// put HEX in LineBuf
			if ((c < ' ') || (c > 126)) c = '.';	// mask out non-printable chars
			LineBuf[9 + 3 * BYTES_LINE + j] = c;	// ASCII part of dump
		}
		LineBuf[9 + 4 * BYTES_LINE] = 0;	// string delimiter after last char
		printf(LineBuf);
		printf(CRLF);
	}
	return Addr + 8*16;
}	// end of dumpRAM


int8_t getArgs(void)
{
uint8_t c;
uint8_t prev_wasSpace = 1;	// flag if previous char was space
uint8_t n = 0;	// number of found args
uint8_t nCiph = 0;	// number of digits
	while (ArgPtr < LinePtr)
	{
		c = LineBuf[ArgPtr++];
		if (c > ' ') {
			if (prev_wasSpace) {
				prev_wasSpace = 0;
				if (++n > 3) return ERR;	// max 3 arguments allowed
				Arg[n-1] = 0;
				nCiph = 0;	// number of hex digits
			}
			if ((c < '0') || (c > 'F') || ((c > '9') && (c < 'A'))) {
				return ERR;	// error: no valid hex char
			}
			// conversion ASCII to 1 hex digit:
			if (c >= 'A') c = c - 7;
			c = c - 48;
			Arg[n-1] = (Arg[n-1] << 4) + c;
			if (++nCiph > 4) return ERR;	// number too long
		}
		if ((c == ' ') && (prev_wasSpace == 0)) prev_wasSpace = 1;
	}
	return n;	// 0,1,2,3 possible
}	// end of getArgs

void Bin2Hex(char *here,uint16_t number,uint8_t digits)
{
uint8_t c;
	for (uint8_t i=0;i<digits;i++) {
		c = (number >> ((digits-i-1)*4)) & 0x0F;
		c = c + '0';
		if (c > '9') c = c + 7;
		*(here+i) = c;
	}
}	// end of Bin2Hex

void clearLineBuffer()
{
uint8_t i;
	for (i=0;i<80;i++) LineBuf[i] = ' ';
	LineBuf[4] = ':';
	LineBuf[80] = 0;	// string terminator
}

uint8_t convertHexByte(uint8_t *here)
{
uint8_t x;
uint8_t c;
	x = *here++;	// high nibble
	if (x >= 'A') x = x - 7;
	x = x - 48;
	c = x << 4;
	x = *here;		// low nibble
	if (x >= 'A') x = x - 7;
	x = x - 48;
	c = c + x;
	return c;
}


void record(void)
{
	strcpy(filename,"            ");	// empty file name just in case
	if (RLEmode) RLErecord();
	else KCrecord();
}

void KCrecord(void)
{
uint8_t byte,tc;
uint8_t i,j;
uint8_t CheckSum;
uint8_t RecErr = FALSE;
	eraseRAM();			// empty header block
	sermem_reset();		// reset sermem usage counter
	isTAP = 16;			// we will use the TAP format for recording now
	prepareTAPheader();	// prepare the 16 Bytes long TAP header
	REC_LED_ON;
	cli();
	rleCounter = 0;
	sei();
	CTC0_INT_ON;			// activate Recording CTC interrupt
	while (AUX == 0) {	// wait for KC85 to begin, for KC87 AUX is always 1 
		if (breakFlag) {
			REC_LED_OFF;
			CTC0_INT_OFF;
			pending = 0;		// allow next key input action
			return;				// and stop recording action
		}
	}
	do {						// main recording loop
		CheckSum = 0;	// initialize for next data block
		for (i=0;i<130;i++) {	// get next data block
			byte = 0;		// prepare to collect up bits
			do {
				tc = waitEdge();
				if (breakFlag) break;
			} while ((tc < 130) || (tc > 190));	// wait for Byte separator
			for (j=0;j<8;j++) {
				waitEdge();
				if (breakFlag) break;
				tc = waitEdge();
				byte = byte >> 1;
				if (tc > 70) byte |= 0x80;	// collect "1" Bits
			}	// next j
			if (breakFlag) break;
			if (i < 129) {			// leave block number out from checksum
				if (i > 0) CheckSum += byte;	// exclude blk# from checksum
				if (RAMcounter < MEMSIZE) recMem[RAMcounter] = byte;	// catch header in internal Ram
				sermem_writeByte(byte);		// write to external memory
				RAMcounter++;
			}
			else {	// last byte of block is checksum
				if (CheckSum != byte) RecErr = TRUE;
			}		
		}	// next i
		if (breakFlag) break;			// stop recording if BRK was pressed
		for (i=0;i<10;i++) waitEdge();	// read - ignore some edges
	} while (AUX && (breakFlag == 0) && (RecErr == FALSE));	// end of main recording loop
	
	if (RecErr == FALSE) {				// everything appears to be right, write data to SD-Card
		if (RAMcounter > (isTAP + 129)) {	// minimum one block expected for a valid file!
			KCgetFileName();			// extract KC file name from header		
			dispFileName(filename);		// and display it
			writeFile_SD(filename);
		}
	}
	else {	// a checksum error occured
		lcd_string_xy(2,1,"RECORD ERR!");
		do {
			REC_LED_ON;
			_delay_ms(300);
			REC_LED_OFF;
			_delay_ms(300);
		} while (breakFlag == 0);		// blink until End key was pressed
		eraseRAM();						// delete erroneous data from memory
	}
	// finally switch REC LED off, deactivate CTC interrupt and return to input mode
	REC_LED_OFF;
	CTC0_INT_OFF;
	pending = 0;		// allow next key input action
}	// end of KCrecord

void prepareTAPheader(void)
{
uint8_t th[] = "\xC3" "KC-TAPE by AF. ";
uint8_t i=0;
	while (th[i]) {
		sermem_writeByte(th[i]);
		recMem[i] = th[i];
		i++;
	}
	RAMcounter = i;					// set Ram Counter to next free Byte
}

void writeFile_SD(char *filename)
{
uint8_t 	buf[128];
uint32_t	nblocks;
uint8_t		lastblock;	
uint32_t	i;
uint8_t		j;
	sermem_reset();		// reset serial mem to position zero
	nblocks = RAMcounter / 128;
	lastblock = RAMcounter % 128;
	Remove(filename);	// file operation: delete file
	Fopen(filename,F_WRITE);
	for (i=0;i<nblocks;i++) {
		for (j=0;j<128;j++) buf[j] = sermem_readByte();
		Fwrite(buf,128);
	}
	if (lastblock) {	// only if there are remaining bytes...
		for (j=0;j<lastblock;j++) buf[j] = sermem_readByte();
		Fwrite(buf,lastblock);
	}
	Fclose();
}	// end of writeFile_SD

void KCgetFileName(void)	// get file name for the purpose of writing it to SD-Card
{
uint8_t i;
uint8_t BlockLen = 130;
uint8_t offset = 0;
 for (i=0;i<11;i++) filename[i] = ' ';	// first initialize empty file name

	 if (recMem[0] == 0xC3) {	// TAP file
	 	offset = 16;		
		BlockLen = 129;
	 }
	 if (recMem[offset] == 0) {	// KC87 file

		if(isBasic(recMem+offset+BlockLen+1))	{	// Basic has precedence over machine code
			offset += BlockLen;
		 	for (i=0;i<8;i++) {
				if (recMem[4+i+offset] >= ' ') filename[i] = recMem[4+i+offset];
			}
			for (i=0;i<3;i++) {
				if ((recMem[1+i+offset] & 0x7F) >= ' ') filename [8+i] = recMem[1+i+offset] & 0x7F;
			}
		}
		else {		// must be KC87 machine code
			for (i=0;i<11;i++) {
				if	(recMem[offset+1+i]) filename [i] = recMem[offset+1+i];
			}
		}
	 }
	 else {		// KC85 file
		if (recMem[offset + 1] & 0x80) {	// KC85 Basic file or listing
		 	for (i=0;i<8;i++) {
				if (recMem[4+i+offset] >= ' ') filename[i] = recMem[4+i+offset];
			}
			for (i=0;i<3;i++) {
				if ((recMem[1+i+offset] & 0x7F) >= ' ') filename [8+i] = recMem[1+i+offset] & 0x7F;
			}
		}
	 	else {	// KC85 machine code
			for (i=0;i<8;i++) {
				if (recMem[offset+1+i]) filename [i] = recMem[offset+1+i];
			}
		}
	 }
}	// end of KCgetFileName


uint8_t isBasic(uint8_t *there) 
{
uint8_t i;
uint8_t rv = TRUE;
uint8_t byte = *there;
	for (i=1;i<3;i++) {
		if (*(there + i) != byte) rv = FALSE;		// all 3 bytes must be equal
		if ((*(there + i) & 0x80) == 0) rv = FALSE;	// bit 7 must be set
	}
	return rv;		// TRUE if Basic prog or listing found, otherwise FALSE
}

// wait for level change of REC signal, return counter sum from last 2 events
uint8_t waitEdge(void)
{
static uint8_t prev = 0;
uint8_t level;
uint8_t	ticks;
uint8_t x;
	level = REC;
	do
	{
	} while ((level == REC) && AUX && (breakFlag == 0));	// wait for level change or end of recording

	cli();						// no interrups
	if (rleCounter > 255UL) {	// invalid data
		prev = ticks = 0;
	}
	else ticks = rleCounter;	// if valid, get counter value
	rleCounter = 0UL;			// reset counter for next time
	sei();						// allow interrupts again

	x = prev + ticks;
	prev = ticks;
	PORTA ^= 1<<REPLAY;					// reflect incoming Edge through beeper
	return x;
}	// end of waitEdge


void replay(void)
{
	if (RLEmode) RLEreplay();
	else KCreplay();
}

void KCreplay(void)			// KC mode
{
uint32_t	pos = 0;		// position in data buffer
uint8_t		i;
	if (recMem[0] == 0xC3) isTAP = 16;
	else isTAP = 0;
	sermem_reset();			// reset serial mem to position zero
//
	if (RAMcounter == 0) {
		pending = 0;		// allow next input
		return;				// exit from replay
	}
//
	PLAY_LED_ON;
	CTC2_INT_ON;
	beep(2000);			// initial beep, normally not required
	if (isTAP) {		// TAP file
		for (i=0;i<16;i++) sermem_readByte();
		pos += 16;
		while (pos < RAMcounter) {
			playBlock(pos);
			pos += 129;	
			if (breakFlag) break;
		}	
	}							// raw file
	else {
		while (pos < RAMcounter) {
			playBlock(pos);
			pos += 130;	
			if (breakFlag) break;
		}
	}
	PORTA ^= 1<<REPLAY;		//flip output one last time
	_delay_ms(100);			//... and wait a bit
	PORTA &= ~(1<<REPLAY);	//set PLAY voltage to GND
	CTC2_INT_OFF;
	PLAY_LED_OFF;
	pending = 0;
}	// end of KCreplay

void beep(uint16_t n)
{
uint16_t i;
	for (i=0;i<2*n;i++) {
		downCounter = TIMECONST2;
		while (downCounter) { };	// same timing mechanism as with record
		PORTA ^= 1<<REPLAY;
	}
}	// end of beep

void playBlock(uint16_t pos)
{
uint8_t i,j;
uint8_t b;
uint8_t checksum = 0;
uint8_t BlkLen = 130;
uint8_t offset = 0;
	if (isTAP) {
		offset = 16;
		BlkLen = 129;
	}
	if (recMem[offset] == 0) offset += BlkLen;	// KC87 program, begins with block# 0
	if (recMem[offset + 1] == 0xD5) beep(1800);	// list#1 requires long beep
	else beep(180);
	for (i=0;i<130;i++) {
		// byte separator
		downCounter = TIMECONST3;
		while (downCounter) { };	// same timing mechanism as with record
		PORTA ^= 1<<REPLAY;
		downCounter = TIMECONST3;
		while (downCounter) { };	// same timing mechanism as with record
		PORTA ^= 1<<REPLAY;
// for TAP calculate checksum, otherwise read it in:
		if (i == 129) {
			if (recMem[0] == 0xC3) b = checksum;
			else b = sermem_readByte();
		}
		else {
			b = sermem_readByte();
			if (i) checksum += b;	// omit block#
		}
//	
		for (j=0;j<8;j++) {
			if (b & 1) {	// 1 Bit
				downCounter = TIMECONST2;
				while (downCounter) { };	// same timing mechanism as with record
				PORTA ^= 1<<REPLAY;
				downCounter = TIMECONST2;
				while (downCounter) { };	// same timing mechanism as with record
				PORTA ^= 1<<REPLAY;
			}
			else {			// 0 Bit
				_delay_us(TIMECONST1);
				downCounter = TIMECONST1;
				while (downCounter) { };	// same timing mechanism as with record
				PORTA ^= 1<<REPLAY;
				downCounter = TIMECONST1;
				while (downCounter) { };	// same timing mechanism as with record
				PORTA ^= 1<<REPLAY;
			}
			b = b >> 1;		// right shift Byte
		}
	}
	downCounter = TIMECONST3;
	while (downCounter) { };	// same timing mechanism as with record
	PORTA ^= 1<<REPLAY;
	_delay_ms(20);		// 20 milliseconds gap after each block 
}	// end of playBlock

void displayInitScreen(void)	// show general info for some seconds at power on
{
		lcd_clear();
		lcd_string_xy(0,0,"KC85 Recorder");
		lcd_string_xy(0,1,"-------------");
		lcd_string_xy(0,2,"Firmware Version");
		lcd_string_xy(0,3,VERSION);
		_delay_ms(1500);
}	// end of displayInitScreen

void displayMenu(char * c)
{
	lcd_clear();
	lcd_string_xy(0,3,c);
}	// end of dispMenu

void dispFileName(char *filename)
{
	lcd_string_xy(2,1,filename);
}	// end of dispFileName
