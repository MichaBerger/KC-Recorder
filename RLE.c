#include "main.h"
#include "RLE.h"
#include "SD_CARD/dos.h"

// local prototypes
uint32_t getRLEword();
void	 RLEnewFileName(char *filename);
void 	 RLEdispFileName(char *filename);
void 	createCSWheader(uint32_t nEdges);

// variables
volatile uint32_t	rleVal;
volatile uint8_t	ChangeFlag;

ISR (PCINT0_vect)	// Pin Change Int for "record" pin PA2 
{
	rleVal = rleCounter;	// save RLE counter value
	rleCounter = 0;			// ... and reset the counter
	ChangeFlag = TRUE;		// set flag for main program: level change has occured
}


void RLErecord(void)
{
uint32_t nEdges = 0;	// number of signal level changes
uint8_t  i;
	eraseRAM();			// empty RAM header
	RAMcounter = 32;	// reserve 32 Bytes lengt for CSW header

	sermem_reset();		// reset sermem usage counter
	for (i=0;i<32;i++) sermem_readByte();	// skip 32 bytes of CSW 1.01 header area

	RLEnewFileName(filename);	// generate a new nnnn.csw file name
	dispFileName(filename);	// and display it
	REC_LED_ON;
	cli();
	rleCounter = 0;
	sei();
	CTC0_INT_ON;			// activate CTC interrupt
	REC_INT_ON;			// activate Record pin change INT

	do {						// main recording loop
		while (ChangeFlag == FALSE) {
			if (breakFlag) break;
		};	// wait for a record event
		if (breakFlag) break;	// no further action
		ChangeFlag = FALSE;		// reset pin change flag for next time
		PORTA ^= 1<<REPLAY;		// reflect incoming Edge through beeper
		nEdges++;				// one more signal level change
		rleVal >>= 1;			// convert from 88200 /s to 44100 /s
		if (rleVal < 256) {			// write a byte
			sermem_writeByte(rleVal);
			RAMcounter++;
		}
		else {					// write a long
			sermem_writeByte(0);	// 0 initializes a long
			for (i=0;i<4;i++) {
				sermem_writeByte((uint8_t) rleVal);
				rleVal = rleVal >> 8;
			}
			RAMcounter += 5;	// zero byte + long var
		}

	} while (1);	// end of main recording loop
	
	// finally switch REC LED off, deactivate CTC and Record Pin interrupts and return to input mode
	REC_LED_OFF;
	CTC0_INT_OFF;
	REC_INT_OFF;
	createCSWheader(nEdges); // generate the CSW header in the serial memory
	writeFile_SD(filename);	// and write it to SD card
	pending = 0;		// allow next key input action
}	// end of RLErecord


void RLEreplay(void)
{
uint32_t	i;
uint32_t	replayCounter;
	sermem_reset();			// reset serial mem to position zero
//
	if (RAMcounter == 0) {
		pending = 0;		// allow next input
		return;				// and exit from replay
	}
//
	for (i=0;i<32;i++) sermem_readByte();	// skip the 32 bytes long CSW 1.01 header
	replayCounter = 32;

	nextRLEword = getRLEword();	// prepare next portion of data for CTC int
	downCounter = 1000;
	replayCounter += 5;
	reqNextFlag = FALSE;

	PLAY_LED_ON;
	CTC2_INT_ON;				// activate CTC interrupt
	do {	
		while (reqNextFlag == FALSE) {
			if (breakFlag) break;
		};	// wait for the ready signal from CTC int
		if (breakFlag) break;

		nextRLEword = getRLEword();
		reqNextFlag = FALSE;

		if (nextRLEword < 256) replayCounter++;
		else replayCounter += 5;
	} while (replayCounter < RAMcounter);
	CTC2_INT_OFF;	// deactivate CTC interrupt
	PLAY_LED_OFF;
	pending = 0;
}	// end of RLEreplay

// ************* Local Functions **************

uint32_t getRLEword()
{
uint32_t 	x;
uint8_t		i;
	x = sermem_readByte(); 				
	if (x == 0) {
		for (i=0;i<4;i++) {
			x >>= 8;
			x |= (uint32_t) sermem_readByte() << 24;
		}
	}
	return x;
}	// end of getRLEword

// generate the next free file name and deposit it in string "filename"
void RLEnewFileName(char *filename)
{
uint16_t	freeNum;
				//   12345678.
	strcpy(filename,"        .csw");	// initialize pattern for 8+3 file name
	freeNum = numFiles;
	do {
		freeNum++;
		Bin2Hex(filename,freeNum,4);		// next potential file number
	} while (FindName(filename) == FULL_MATCH);
	numFiles++;							// increment number of files
}

// create the 32 Bytes long CSW 1.01 header
void createCSWheader(uint32_t nEdges)
{
const char text[] = "Compressed Square Wave";
const uint16_t SampleRate = 44100u;
uint8_t i;
	sermem_reset();
	for (i=0;i<22;i++) sermem_writeByte(text[i]);	// write csw header string
	sermem_writeByte(0x1A);							// string terminator code
	sermem_writeByte(1);							// major version
	sermem_writeByte(1);							// minor version

	sermem_writeByte(SampleRate);					// little endian notation of the sample rate
	sermem_writeByte(SampleRate >> 8);
	sermem_writeByte(0x01);							// ordinary RLE compression
	sermem_writeByte(0x00);							// initial polarity LOW ???
	for (i=0;i<3;i++) sermem_writeByte(0);			// 3 reserved bytes, set to zero
}	// end of createCSWheader

