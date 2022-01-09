//#########################################################################
// File: DOSDEFS.H
//
// Central DOS configuration file for this project
//
//#########################################################################
// Last change: 22.02.2007
//#########################################################################
// Compiler: AVR-GCC 3.4.5
//#########################################################################

#ifndef __DOSDEFS_H
#define __DOSDEFS_H

//define only ONE of these !
//#define COMPACTFLASH_CARD
#define MMC_CARD_SPI //SD_CARD_SPI too !

//maybe later ;)
//#define HD_DRIVE //not used yet

// spare program memory by deciding if we want to read, write or both
//#undef DOS_READ	//undefine this if you don't want to read files with Fread()
//#undef DOS_WRITE	//undefine this if you don't want to write files with Fwrite()
                        //deleting files is also deactivated
//#undef DOS_DELETE	//undefine this if you don't want to delete files
//#undef DOS_READ_RAW	//undefine this if you don't want to read files with ReadFileRaw()

//#undef DOS_MKDIR	//undefine this if you don't want to make subdirectories
//#undef DOS_CHDIR	//undefine this if you don't want to go into subdirectories
#undef DOS_RENAME	//undefine this if you don't want to rename files

//#undef DOS_FSEEK	//undefine this if you don't want to seek in files

// spare program memory by deciding if we want to use FAT12, FAT16, FAT32.
// comment out FAT types not used. NOT recommended if you don't know the
// FAT type of your drive !
//#undef USE_FAT12	//undefine this if you don't want to use FAT12
//#undef USE_FAT16	//undefine this if you don't want to use FAT16
//#undef USE_FAT32	//undefine this if you don't want to use FAT32

//#undef USE_FATBUFFER	//undefine this if you don't want to use a FAT buffer
                        //needs 517 Bytes of RAM ! 

//#undef USE_FINDFILE     //undefine this if you don't want to use Findfirst(), Findnext()
#undef USE_FINDLONG     //undefine this if you don't want to get long filenames
			//from Findfirst(); Findnext();

#undef USE_DRIVEFREE	//undefine this if you don't want to get used and free space of your drive

// MMC/SD card debug options
//#define MMC_DEBUG_MAX_BUSY_WRITE //activate counting maximum busy time for writes (timer ISR required !)
//#define MMC_DEBUG_IDENTIFY //activate debug output for MMCIdentify() via printf()
//#define MMC_DEBUG_SECTORS_READ //activate debug output via printf()
//#define MMC_DEBUG_SECTORS_WRITE //activate debug output via printf()
//#define MMC_DEBUG_COMMANDS //activate debug output via printf()
//#define MMC_DEBUG_CMD0_TIMEOUT //activate debug output via printf()
//#define MMC_DEBUG_CMD1_TIMEOUT //activate debug output via printf()

// These defines are for FAT debugging only. You don't need them.
// Activating all three options takes about 1.5kB of flash.
// So be careful on devices with small flash memory !
//#define FAT_DEBUG_SHOW_FAT_INFO //activate FAT information output via printf() or puts() in GetDriveInformation()
//#define FAT_DEBUG_CLUSTERS // show cluster numbers read/write access via printf()
//#define FAT_DEBUG_RW_HITS // show FAT sectors read/write access summary when calling Fclose(), via printf()

#endif //__DOSDEFS_H
