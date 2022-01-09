#include "main.h"
#include "filework.h"
#include "SD_CARD/dos.h"

void selectFile(void);
uint8_t loadFile(void);
void getFileList(void);
void disp3Files(void);
void waitforKeyReleased(uint8_t);
uint8_t	waitforKeyPressed();
void deleteDialog(void);

extern void replay(void);
extern void displayMenu(char*);

uint16_t dirPos = 0;	// current position in file list
uint8_t  dispPos = 0;	// current position on display
uint8_t keyFlag;
uint8_t repeatFlag;
char fileList[13*MAXFILES];
char dirList[13*MAXDIRS];
uint16_t numFiles;
uint16_t numDirs;
uint16_t numEntries;

void FileDialog()
{
	repeatFlag = 0;
	displayMenu("Del Load Down Up");
	if (Findfirst()) {	// if at least one file is there
		selectFile();
	}
	else {	// no file
		lcd_string_xy(0,1," No Files found");
		_delay_ms(1500);
	}
}	// end of FileDialog

void selectFile()
{
	PCMSK3 = 0;	// disable key interrupts

	do {
		getFileList();
		do {
			disp3Files();
			waitforKeyReleased(1);
			keyFlag = waitforKeyPressed();
			if (keyFlag & (1<<DOWN_KEY)) {
				if (dirPos < (numEntries - 1)) {
					dirPos++;	// only down if one more file exists
					dispPos++;
					if (dispPos > 2) dispPos = 2;
				}	
			}
			else {
				if (keyFlag & (1<<UP_KEY)) {
					if (dirPos > 0) {
						dirPos--;
						if (dispPos > 0) dispPos--;
					}
				}
				else {
					if (keyFlag & (1<<DEL_KEY)) {
						deleteDialog();
						if (numEntries == 0) {
							PCMSK3 = KEYMASK;	// re-enable key interrupts
							breakFlag = 0;		// suppress the break flag here
							waitforKeyReleased(0);
							return;				// and abandon file dialog
						}
					}
				}
			}
		} while ((keyFlag & (1<<LOAD_KEY)) == 0);
	} while (0 == loadFile());			// until a file was loaded

	PCMSK3 = KEYMASK;	// re-enable key interrupts
	breakFlag = 0;		// suppress the break flag here
	waitforKeyReleased(0);	// continue only after key release
	replay();	// dirPos 0 means "back to recorder"
}	// end of selectFile

void getFileList()
{
uint16_t	i;
	for (i=0;i<13*MAXFILES;i++) fileList[i] = 0;	// reinit file list
	for (i=0;i<13*MAXDIRS;i++) 	dirList[i] = 0;		// reinit dir list
	numFiles = numEntries = 0;
	strcpy(dirList,BACK_TO_REC);	// exit option at top of dir entries
	numDirs = 1;
	if (Findfirst()) {	// act only if at least one item found
		do {
			if (ffblk.ff_attr == 0) {	// it is a file!
				strcpy((fileList + 13 * numFiles),ffblk.ff_name);
				strlwr((fileList + 13 * numFiles));
				numFiles++;
			}
			else {						// it is a dir!
				if (strcmp(".",ffblk.ff_name)) {	// skip the . entry
					strcpy((dirList + 13 * numDirs),ffblk.ff_name);
					strupr((dirList + 13 * numDirs));
					numDirs++;
				}
			}
		} while (Findnext());	// until all entries read
		numEntries = numDirs + numFiles;
	}
}	// end of getFileList

void disp3Files()
{
char item[] = "            ";
int16_t topPos;
uint8_t i;
	topPos = dirPos - dispPos;	// filename for first display line
	for (i=0;i<3;i++) {
		lcd_string_xy(0,i,"                ");
		if ((topPos + i) < numEntries) {
			if ((topPos + i) < numDirs) {
				strcpy(item,(dirList + 13*(topPos+i)));
			}
			else {
				strcpy(item,(fileList + 13*(topPos+i-numDirs)));
			}
			lcd_string_xy(2,i,item);
		}
	}
	lcd_string_xy(1,dispPos,">");
}	// end of disp3Files

// debounced key release handling
void waitforKeyReleased(uint8_t withRepeat)
{
uint16_t i;
uint16_t repCount = 0;
	if (repeatFlag == 0) {
		do {
			keyFlag = 0;
			for (i=0;i<2000;i++) keyFlag |= ~(DDRD | PIND) & KEYMASK;
			if ((repCount++ > 700) && withRepeat) {
				repeatFlag = 1;	// key repeat
				return;
			}	
		} while (keyFlag);
	}
	else {
		// release repeat status if key not constantly pressed:
		keyFlag = 0xFF;
		for (i=0;i<20000;i++) keyFlag &= ~(DDRD | PIND) & KEYMASK;
		if (keyFlag == 0) repeatFlag = 0; 
	}
}	

// debounced key press handling
uint8_t	waitforKeyPressed()
{
uint8_t key, prevkey;
uint16_t waiting = 1000;
	do {
		key = ~(DDRD | PIND) & KEYMASK;
		if (key && (key == prevkey)) waiting--;
		else waiting = 1000;
		prevkey = key;
	} while (waiting);
	return key;
}

uint8_t loadFile()
{
uint32_t i;
uint8_t	 j;
uint8_t  buff[128];
uint32_t nblocks;
uint8_t  lastblock;
	RAMcounter = 0;	// nothing valid yet loaded
	strcpy(filename,"            ");	// reset file name  
	for (i=0;i<MEMSIZE;i++) recMem[i] = 0;	// delete file header just in case
	if (dirPos == 0) {
		return 1;
	}
	if (dirPos >= numDirs) {	// perform load only if it is a file
		displayMenu("Rec Play End DIR");
		strcpy(filename,(fileList + 13 * (dirPos-numDirs)));
		dispFileName(filename);
		PLAY_LED_ON;	// already switch Play LED on, because Replay always follows
		Fopen(filename,F_READ);
		RAMcounter = FileSize;
		nblocks = FileSize / 128;	// # of complete blocks
		lastblock = FileSize % 128;	// byte size of last block
		sermem_reset();		// set serial memory to pos zero
		for (i=0;i<nblocks;i++) {
			Fread(buff,128);
			for (j=0;j<128;j++) sermem_writeByte(buff[j]);
		}	
		if (lastblock) {		// if there is a remainder
			Fread(buff,lastblock);
			for (j=0;j<lastblock;j++) sermem_writeByte(buff[j]);	
		}
		sermem_reset();
		for (j=0;j<MEMSIZE;j++) recMem[j] = sermem_readByte();
/*		for (i=0;i<RAMcounter;i++) {
			Fread(&b,1);
			if (i < MEMSIZE) recMem[i] = b;
			sermem_writeByte(b);
		}
*/
		Fclose();
		isTAP = 0;
		if (recMem[0] == 0xC3) isTAP = 16;	// offset for TAP files
		return 1;	// file loaded
	}
	else {	// if it is a dir:
		if (strcmp("RLE",(dirList + 13 * dirPos)) == 0) RLEmode = TRUE;	// if in RLE folder then set RLEmode flag
		else RLEmode = FALSE;												// for all other locations normal KC mode
		Chdir((dirList + 13 * dirPos));		// 
		dirPos = dispPos = 0;		// set cursor to top of new dir
		return 0;	// dir changed
	}
}	// end of loadFile

void deleteDialog()
{
uint8_t key;
uint16_t i;
uint8_t j;
	if (dirPos >= numDirs) {		// only allow deletion of files, not directories!	
		if (numEntries > 0) {	// delete makes only sense if at least one prog exists
			displayMenu("Yes  No         ");
			lcd_string_xy(2,1,"Delete File?");
			repeatFlag = 0;
			waitforKeyReleased(0);
			key = waitforKeyPressed();
			if (key == (1<<YES_KEY)) {
				Remove((fileList + 13 * (dirPos-numDirs)));	// delete file on SD-Card
				for (i=(dirPos-numDirs);i<(numEntries-numDirs);i++) {
					for (j=0;j<13;j++) fileList[13*i+j] = fileList[13*i+j+13];
				}
				numFiles--;							// one program less
				numEntries = numDirs + numFiles;	// update # of entries count
				if (dirPos == numEntries) {
					dirPos--;	// if last entry deleted go back one entry
					if (dispPos) dispPos--;
				}
			}
			waitforKeyReleased(0);
			displayMenu("Del Load Down Up");	// finally restore file dialog menu
		}
	}
}	// end of deleteDialog
