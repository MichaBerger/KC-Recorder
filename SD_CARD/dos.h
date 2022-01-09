/*! \file "dos.h" \brief DOS-Definitions */
///	\ingroup multifat
///	\defgroup DOS DOS-Functions (dos.h)
//#########################################################################
// File: dos.h
//
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
// 18.02.2007 Changed F_READ to 'r', F_WRITE to 'w'
//            Now you can do Fopen("mydata.txt",'w');    ;)
//
//#########################################################################
// Last change: 23.08.2007
//#########################################################################
// Compiler: AVR-GCC 4.1.1
//#########################################################################
//@{

#ifndef __DOS_H
#define __DOS_H


// Do it in dos.h or make an avrtypes.h and include it here
 typedef unsigned char 	U8;  // one byte
 typedef char 		S8;  // one byte signed
 typedef unsigned int 	U16; // two bytes
 typedef int 		S16; // two bytes signed
 typedef unsigned long 	U32; // four bytes
 typedef long 		S32; // four bytes signed

// Some defines for more speed optimizations 

//#define FWRITE_SMALL_BUFFERS // You can call Fwrite() with buffers up to 254 bytes only.
                               // Needs less code ! Not really faster for buffers >8

//#define FREAD_SMALL_BUFFERS  // You can call Fread() with buffers up to 254 bytes only.
                               // Needs less code ! Not really faster for buffers >8

// Some more defines for special filesystem handling

#define STRICT_FILESYSTEM_CHECKING // If you define this, some very rare special cases will be
				    // noticed. But this needs more code.
				    // Special cases when files where made on a Win system and
				    // copied to the flash card:
				    //
                                    // Bug found by Michele Ribaudo
				    // Files with zero filesize have no clusters allocated !
				    // Calling Remove() and Fopen() may hang up the system.
				    // If you define STRICT_FILESYSTEM_CHECKING opening a
				    // zero length file for reading or writing gives an error only.
				    // You can remove a zero length file !  
				    //
				    // You don't need this define if all files where made with
				    // my FAT system. Even if filesize is zero.
				    // If unsure keep it defined !

//fopen flags
#define F_CLOSED	0
#define F_READ		'r'
#define F_WRITE		'w'

#define F_ERROR		0 // dir/file operation failed
#define F_OK		1 // dir/file operation successfull

#ifndef SEEK_SET
 #define	SEEK_SET	0	/* set file offset to offset */
#endif
#ifndef SEEK_CUR
 #define	SEEK_CUR	1	/* set file offset to current plus offset */
#endif
#ifndef SEEK_END
 #define	SEEK_END	2	/* set file offset to EOF plus offset */
#endif

// #undef defines below in "dosdefs.h" if you don't need them
// spare program memory by deciding if you want to read, write or both
#define DOS_READ	//define this if you want to read files
#define DOS_WRITE	//define this if you want to write files
#define DOS_DELETE	//define this if you want to delete files
#define DOS_READ_RAW	//define this if you want to read files with ReadFileRaw()

#define DOS_CHDIR	//define this if you want to go into subdirectories
#define DOS_MKDIR	//define this if you want to make subdirectories
#define DOS_RENAME	//define this if you want to rename files

#define DOS_FSEEK       //define this if you want to seek in files (reading only)
                        
// spare program memory by deciding if we want to use FAT12, FAT16, FAT32.
// don't touch if you don't know the FAT type of your drive !
#define USE_FAT12	//define this if you want to use FAT12
#define USE_FAT16	//define this if you want to use FAT16
#define USE_FAT32	//define this if you want to use FAT32

#define USE_FATBUFFER	//define this if you want to use a FAT buffer
                        //needs 517 Bytes of RAM ! 

#define USE_FINDFILE	//define this if you want to use findfirst(); findnext();
#define USE_FINDLONG    //define this if you want to get long filenames
			//from findfirst(); findnext();

#define USE_DRIVEFREE   //define this if you want to get free and used space of your drive


#include "dosdefs.h"  // keep the line at this place ! don't move down or delete 

#ifdef USE_FATBUFFER
 #define FAT_IO_BUFFER	fatbuf
#else
 #define FAT_IO_BUFFER	iob
#endif

extern U8 Fopen(char *name, U8 flag);
extern void Fclose(void);

#ifdef DOS_READ
#ifdef FREAD_SMALL_BUFFERS
 extern U8 Fread(U8 *buf, U8 count);
#else
 extern U16 Fread(U8 *buf, U16 count);
#endif
extern U8 Fgetc(void);
#endif

#ifdef DOS_WRITE
#ifdef FWRITE_SMALL_BUFFERS
 extern U8 Fwrite(U8 *buf, U8 count);
#else
 extern U16 Fwrite(U8 *buf, U16 count);
#endif
extern U16 Fputc(U8 buf);
#endif

extern void Fflush(void);
extern U8 Remove(char *name);

extern U8 ReadFileRaw(char *name);
extern U8 FindName(char *name);
extern U8 Rename(char *OldName, char *NewName);

// ###Seek Funktion
extern U8 Fseek(S32 offset, U8 mode);

#define Filelength()	FileSize

#ifdef USE_FAT32
 extern U32 FileFirstCluster;
 extern U32 FileCurrentCluster;
#else
 extern U16 FileFirstCluster;
 extern U16 FileCurrentCluster;
#endif

extern U8 FileClusterSectorOffset;
extern U32 FileFirstClusterSector;
extern U32 FileClusterCount;
extern U32 FileDirSector;
extern U8 FileDirOffset;
extern U32 FileSize;
extern U32 FilePosition;
extern U8 FileFlag;
extern U8 FileAttr;
extern char FileName[];

//this is for easier and faster converting from byte arrays to UINT, ULONG
//ui and ul share the same memory space
union Convert {
 U16 ui;
 U32 ul;
};

#ifdef COMPACTFLASH_CARD
 #include "compact.h"
#endif

#ifdef MMC_CARD_SPI
 #include "mmc_spi.h"
#endif

#include "fat.h"

//dir.c
extern U8 Mkdir(char *name);
extern U8 Chdir(char *name);
extern U8 MakeNewFileEntry(void);
extern U8 UpdateFileEntry(void);
extern U8 ScanOneDirectorySector(U32 sector, char *name);
//extern U16 DOSTime(void);
//extern U16 DOSDate(void);

#ifdef USE_FAT32
// extern U8 SearchFreeDirentry(void);
 extern U8 ScanDirectory(char *name);
#else
// extern U8 SearchFreeDirentry(void);
 extern U8 ScanDirectory(char *name);
#endif

extern U8 SearchDirSector(U32 sector);
extern void MakeDirEntryName(char *inname);
extern void ZeroCluster(U32 startsector);

#ifdef USE_FAT32
 extern U32 FirstDirCluster;
#else
 extern U16 FirstDirCluster;
#endif
//dir.c

#ifdef USE_FINDFILE
 #include "find_x.h"
#endif

//drivefree.c
extern U32 drivefree(void);
extern U32 driveused(void);
extern U32 drivesize(void);

#endif //__DOS_H
//@}
