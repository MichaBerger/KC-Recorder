/*! \file dos.c \brief DOS-Functions */
//###########################################################
///	\ingroup singlefat
///	\defgroup DOS DOS-Functions (dos.c)
///	\code #include "dos.h" \endcode
///	\code #include "mmc_spi.h" \endcode
///	\par Uebersicht
//###########################################################
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
// If you like it fast compile with -O2
// If you like it small compile with -Os -mcall-prologues
//
// 24.03.2010 Removed bug in Fseek()
//
// 23.08.2007 Changed FileCurrentSector to FileFirstClusterSector.
//            Add FileClusterSectorOffset every time we need to.
//
// 17.08.2007 Removed SMALL_FWRITE, SMALL_FREAD. So FAST_FWRITE, FAST_FREAD
//            is the only option now. Removed #ifdefs for this too.
//            Code looks much better now.
//
// 10.08.2007 Remove() can delete directorys now.
//            Take care that you only delete EMPTY directorys.
//            Remove() does not clean up for you !
//
// 28.06.2007 Fseek() from Alex ???.
//            Seeking is only available for files open for reading !
//
//#########################################################################
// Last change: 24.03.2010
//#########################################################################
// hk@holger-klabunde.de
// http://www.holger-klabunde.de/index.html
//#########################################################################
// Compiler: avr-gcc 4.1.1
//#########################################################################
//@{
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dos.h"

#if defined (FAT_DEBUG_RW_HITS)
 #include "serial.h" //for testing only
 #include "printf.h" //for testing only
#endif

#ifdef USE_FAT32
 U32 FileFirstCluster=0;     //first cluster of file
 U32 FileCurrentCluster=0;   //actual cluster in use
#else
 U16 FileFirstCluster=0;     //first cluster of file
 U16 FileCurrentCluster=0;   //actual cluster in use
#endif

U8 FileClusterSectorOffset=0; //sectoroffset of cluster in use
U32 FileFirstClusterSector=0;    // first sector of cluster with last data read/written
U32 FileClusterCount=0;     //clusters read (not really)
U32 FileDirSector=0;        //sector with dir entry of file
U8 FileDirOffset=0;        //offset to file entry in FileDirSector
U32 FileSize=0;
U32 FilePosition=0;         //actual position when reading file         
U8 FileFlag=0;		    //read or write
U8 FileAttr=0;		    //file attribute
char FileName[11];

#ifdef DOS_WRITE
#ifdef DOS_DELETE
//###########################################################
/*!\brief Delete a file/directory
 * \param		name	DOS 8.3 name of the file 
 * \return 		F_OK if file/directory could be deleted, F_ERROR if not
 */
U8 Remove(char *name)
//###########################################################
{
#ifdef USE_FAT32
 U32 tmp,tmp1;
#else
 U16 tmp,tmp1;
#endif

 if(FileFlag!=0) return F_ERROR; // don't delete if a file is open

 FileAttr=0xFF; // test if we really find a file or a directory
                
 if(FindName(name)==FULL_MATCH) // Look if file/dir exists
  {

   if(FileAttr != ATTR_FILE && FileAttr != ATTR_DIRECTORY) return F_ERROR; // this was not a file/dir !


#ifdef STRICT_FILESYSTEM_CHECKING
// Bug found by Michele Ribaudo
   if(FileFirstCluster>=2) // Zero length files made by Win have no cluster chain !
    {
#endif

     tmp=FileFirstCluster;

     // free clusters in FAT cluster chain (make zero)
     do
      {
       tmp1=GetNextClusterNumber(tmp); // save next cluster number
       WriteClusterNumber(tmp,0);      // free cluster
       tmp=tmp1;                       // restore next cluster number
      }while(tmp<endofclusterchain);

#ifdef STRICT_FILESYSTEM_CHECKING
    }
#endif

   if(FATStatus>0)
    {
     WriteFATSector(FATCurrentSector,FAT_IO_BUFFER); // write the FAT buffer
     FATStatus=0;
    } 

   FileName[0]=0xE5;   //mark file as deleted. does not affect long filename entrys !
   FileSize=0;         //make filesize 0
   FileFirstCluster=0; //delete first cluster
   FileAttr=0;         //is this necessary ? 
   UpdateFileEntry();
  }
  
 return F_OK;  
}
#endif //DOS_DELETE
#endif //DOS_WRITE

#ifdef DOS_WRITE
//###########################################################
// write one byte to file
// returns 0 if nothing written
// returns 1 if byte is written
//
// Fputc() does not write to CF until a sector is completely
// filled. you can force writing with Fflush() when file should
// keep open, or close the file with Fclose().
//
//###########################################################
/*
U16 Fputc(U8 buf)
{
 return Fwrite(&buf,1); //the easy way :)
//maybe its faster to return U8
}
*/

//###########################################################
/*!\brief Write to a file
 * \param		buf	Buffer containing your data
 * \param		count	Number of bytes to be written
 * \return 		Number of bytes written to the file
 *
 * If number of bytes written is 0, your disk is full.

 Fwrite() does not write to CF until a sector is completely
 filled. You can force writing with Fflush() when file should
 keep open, or close the file with Fclose().

 */
//###########################################################
#ifdef FWRITE_SMALL_BUFFERS
 U8 Fwrite(U8 *buf, U8 count)
#else
 U16 Fwrite(U8 *buf, U16 count)
#endif
//###########################################################
{
 U8 secoffset;
 U32 tmp;
 U16 buffoffset;

#ifdef FWRITE_SMALL_BUFFERS
 U8 tmp2;
 U8 remaining;
 U8 bytecount;
#else
 U16 tmp2;
 U16 remaining;
 U16 bytecount;
#endif
 
 if(FileFlag!=F_WRITE) return 0; //don't write if file is closed
                                 //or open for reading !

 bytecount=0;
 remaining = count;
 
 while(remaining)
  {
   tmp=FileSize;         //next write position
   tmp-= FileClusterCount; //calc byte position in cluster
   
   if(tmp >= BytesPerCluster)//is position in new cluster ?
    {
     WriteFileSector(FileFirstClusterSector + FileClusterSectorOffset,iob); //write last sector
     FileCurrentCluster=AllocCluster(FileCurrentCluster); //alloc new cluster
     if(FileCurrentCluster==DISK_FULL)
      {
       return bytecount; //return number of bytes written before disk full
      } 

     #ifndef USE_FATBUFFER // keep it here holgi !?
      iob_status=IOB_DATA; // we have to track this for FClose() and Fflush() !
     #endif

     FileFirstClusterSector=GetFirstSectorOfCluster(FileCurrentCluster); //set new 1st sector of cluster
     FileClusterSectorOffset = 0;

//       FileClusterCount++;                   //update cluster count
     FileClusterCount+=BytesPerCluster;   //update cluster count
     tmp-= BytesPerCluster;               //calc new byte position in cluster
    }

   buffoffset=(U16)tmp; //we loose upper bits here, but it does not matter !
   
//   secoffset=(U8)(buffoffset / BYTE_PER_SEC);  //calc offset from first sector in cluster
   secoffset=(U8)(buffoffset >> 9 );  //calc offset from first sector in cluster

//   buffoffset=(U16)(tmp % BYTE_PER_SEC); //calc offset in sector
   buffoffset = buffoffset & (BYTE_PER_SEC - 1); // tmp % 512 => tmp & (512-1)

   if(FileClusterSectorOffset != secoffset)
    {
     WriteFileSector(FileFirstClusterSector + FileClusterSectorOffset,iob); //write last sector used
     FileClusterSectorOffset = secoffset;
    }

  if((buffoffset + remaining) <= BYTE_PER_SEC) // all bytes can be copied to current sector buffer
   {
     tmp2 = remaining;
     remaining = 0; // All bytes copied
   }
  else // All bytes can NOT be copied to current sector buffer. Copy as much as we can
   {   // to fill the buffer to the end. Then do the loop again with remaining bytes.
     tmp2 = (BYTE_PER_SEC-buffoffset);
     remaining -= tmp2;
   } 

  FileSize += tmp2; //update Filesize
  bytecount += tmp2;

  U8 *p;
  p = &iob[buffoffset];

//  while(tmp2--) *p++ = *buf++;

  do // a little bit faster,smaller than while(tmp2--)
   {
    *p++ = *buf++;
    tmp2--;
   }while(tmp2);

 }// while(remaining)

 return bytecount;
}
#endif //DOS_WRITE

#ifdef DOS_READ
//###########################################################
// read one byte from file
// returns the byte
// NO ERROR CHECKING ! Use FileSize to see if you have
// reached the end of the file !
//###########################################################
/*
U8 Fgetc(void)
{
 U8 by;

 Fread(&by,1);

 return by;
}
*/

//###########################################################
/*!\brief Read from a file
 * \param		buf	Buffer where your data should be stored
 * \param		count	Number of bytes to be read
 * \return 		Number of bytes read from the file
 *
 * If number of bytes read is smaller as count or equal 0, end of file is reached.
 */
#ifdef FREAD_SMALL_BUFFERS
 U8 Fread(U8 *buf, U8 count)
#else
 U16 Fread(U8 *buf, U16 count)
#endif
//###########################################################
{
 U8 secoffset;
 U32 tmp;
 U16 buffoffset;

#ifdef FREAD_SMALL_BUFFERS
 U8 tmp2;
 U8 remaining;
 U8 bytecount;
#else
 U16 tmp2;
 U16 remaining;
 U16 bytecount;
#endif

 if(FileFlag!=F_READ) return 0; // don't read if file is closed
                                // or open for writing !

 bytecount=0;
 remaining=count;

 tmp = FileSize - FilePosition; // get bytes til end of file
 if(tmp < remaining) remaining = tmp; // smaller as we want to read, then read til end of file

 while(remaining)
  {
   tmp=FilePosition;

   tmp-= FileClusterCount; //calc byte position in cluster

   if(tmp >= BytesPerCluster)//is position in current cluster ?
    {
     FileCurrentCluster=GetNextClusterNumber(FileCurrentCluster); //if not get next cluster
     FileFirstClusterSector=GetFirstSectorOfCluster(FileCurrentCluster);//set new 1st sector of cluster
     ReadFileSector(FileFirstClusterSector,iob); //read new sector
     FileClusterSectorOffset = 0;

     FileClusterCount+=BytesPerCluster;   //update cluster count
     tmp-= BytesPerCluster;               //calc new byte position in cluster
    }

   buffoffset=(U16)tmp; //we loose upper bits here, but it does not matter !

//     secoffset=(U8)(buffoffset / BYTE_PER_SEC);  //calc sector offset from first sector in cluster
   secoffset=(U8)(buffoffset >> 9);  //calc sector offset from first sector in cluster

//   buffoffset=(U16)(tmp % BYTE_PER_SEC); //calc offset in sector
   buffoffset = buffoffset & (BYTE_PER_SEC - 1); // tmp % 512 => tmp & (512-1)

   if(FileClusterSectorOffset != secoffset) //new sector ?
    {
     FileClusterSectorOffset = secoffset;
     ReadFileSector(FileFirstClusterSector + FileClusterSectorOffset,iob); //read new sector
    }
      
    if((buffoffset + remaining) <= BYTE_PER_SEC) // All bytes can be copied from current sector buffer
     {
      tmp2 = remaining;
      remaining = 0; // All bytes copied
     }
    else // all bytes can NOT be copied from current sector buffer
     {
       tmp2 = (BYTE_PER_SEC - buffoffset);
       remaining -= tmp2;
     }

    bytecount += tmp2;
    FilePosition += tmp2;

    U8 *p;
    p = &iob[buffoffset];
    
//  while(tmp2--) *buf++ = *p++;

   do // a little bit faster,smaller than while(tmp2--)
    {
     *buf++ = *p++;
     tmp2--;
    }while(tmp2);
   
  } // while(remaining)

 return bytecount;
}
#endif //DOS_READ

#if defined (DOS_READ) || defined (DOS_WRITE)
//###########################################################
/*!\brief Open a file
 * \param		name	8.3 DOS name
 * \param		flag	F_READ or F_WRITE
 * \return 		F_OK if successfull, F_ERROR otherwise
 *
 * Open a file for reading OR writing, NOT both !
 */
U8 Fopen(char *name, U8 flag)
//###########################################################
{
#ifdef DOS_WRITE
 U32 tmp;
#endif //DOS_WRITE


 if(FileFlag!=0) return F_ERROR; //a file is already open !

 FileClusterCount=0;

 #ifdef FAT_DEBUG_RW_HITS
  FATWRHits=0;
  FATRDHits=0;
 #endif
 
#ifdef DOS_READ
 if(flag==F_READ)
  {
   if(FindName(name)==FULL_MATCH) //file MUST exist for reading
    {
     if(FileAttr != ATTR_FILE) return F_ERROR; // this was not a file !

#ifdef STRICT_FILESYSTEM_CHECKING
// Bug found by Michele Ribaudo
     if(FileFirstCluster<2) // no clusters allocated for the file !
      {
       FileFlag=0; //needed for fclose ?
       return F_ERROR; // nothing to read from this file
      } 

#endif

     if(FileSize==0) //nothing to read
      {
       FileFlag=0; //needed for fclose ?
       return F_ERROR; // nothing to read from this file
      } 

     FilePosition=0; //actual read position
     FileCurrentCluster=FileFirstCluster;
     FileFlag=flag; //needed for fclose

     FileFirstClusterSector=GetFirstSectorOfCluster(FileFirstCluster);
     FileClusterSectorOffset = 0;

     ReadFileSector(FileFirstClusterSector,iob); //read first sector of file

     return F_OK;      //give back something above zero
    }
  }
#endif //DOS_READ

#ifdef DOS_WRITE
 if(flag==F_WRITE)
  {
   //if file exists, open it and spool to end of file to append data
   if(FindName(name)==FULL_MATCH)
    {
     if(FileAttr != ATTR_FILE) return F_ERROR; // this was not a file !

#ifdef STRICT_FILESYSTEM_CHECKING
// Bug found by Michele Ribaudo
     if(FileFirstCluster<2) // no clusters allocated for the file !
      {
        // todo: allocate a cluster
        // You can open this file if you do the following:
        // Remove() the file ! Make a new one with Fopen()
        FileFlag=0; //needed for fclose
        return F_ERROR;
      }
#endif

     tmp=FileFirstCluster;
     FileCurrentCluster=FileFirstCluster; //we need this if file is smaller as ONE cluster
     
     while(tmp<endofclusterchain) //go to end of cluster chain
      {
       tmp=GetNextClusterNumber(tmp);
       if(tmp<endofclusterchain)
        {
         FileCurrentCluster=tmp;
         FileClusterCount+=BytesPerCluster;
        } 
      }

     tmp= FileSize - FileClusterCount;
     FileFirstClusterSector = GetFirstSectorOfCluster(FileCurrentCluster); 

//     FileClusterSectorOffset = (U8)(tmp / BYTE_PER_SEC);
     FileClusterSectorOffset = (U8)(tmp >> 9);

     ReadFileSector(FileFirstClusterSector + FileClusterSectorOffset,iob); //read first sector of file
    }
   else //make a new file
    { 
     FileAttr=ATTR_FILE; //needed for MakeNewFileEntry() ! 18.10.2006
     MakeDirEntryName(name); //Split into name and extension
     if(MakeNewFileEntry()) //file does not exist, try to make new file in current directory
      {
       FileCurrentCluster = FileFirstCluster;
       FileFirstClusterSector = GetFirstSectorOfCluster(FileFirstCluster); 
       FileClusterSectorOffset = 0;

       ReadFileSector(FileFirstClusterSector,iob); //read first sector of file
      }
     else
      {
       FileFlag=0; //needed for fclose
       return F_ERROR; //new file could not be made
      }
    }

   FileFlag=flag; //needed for fclose

   return F_OK; //file is open for writing
  }//if(flag==F_WRITE)
#endif //DOS_WRITE
  
 return F_ERROR; //something went wrong  
}

//###########################################################
/*!\brief Close a file
 * \return 		Nothing
 */
void Fclose(void)
//###########################################################
{
#ifdef DOS_READ
// nothing todo
// if(FileFlag==F_READ)
//  {
//  }
#endif //DOS_READ

#ifdef DOS_WRITE
 if(FileFlag==F_WRITE)
  {
   Fflush();  //write last sector used to CF and update filesize, filetime

   #ifdef FAT_DEBUG_RW_HITS
    #ifdef USE_FAT32
     printf("FAT WR Hits %lu ",FATWRHits);
    #else
     printf("FAT WR Hits %u ",FATWRHits);
    #endif
   #endif //#ifdef FAT_DEBUG_RW_HITS
  }


#ifdef USE_FATBUFFER
#else
 iob_status=0;
#endif

#endif //DOS_WRITE

 #ifdef FAT_DEBUG_RW_HITS
  #ifdef USE_FAT32
   printf("FAT RD Hits %lu\n",FATRDHits);
  #else
   printf("FAT RD Hits %u\n",FATRDHits);
  #endif
 #endif //#ifdef FAT_DEBUG_RW_HITS

 FileFlag=0; //a second fclose should do nothing
             //reading and writing disabled

}
#endif //#if defined (DOS_READ) || defined (DOS_WRITE)

#ifdef DOS_WRITE
//###########################################################
/*!\brief Flush a file buffer
 * \return 		Nothing
 *
 Force writing last data written into sectorbuffer to be
 stored into CF without Fclose(). Direntry will also be updated.
 If you use a FAT buffer, it will also be written.
 */
void Fflush(void)
//###########################################################
{
 if(FileFlag==0) return; //don't write if file is closed

#ifdef USE_FATBUFFER
 if(FATStatus>0)
  {
   WriteFATSector(FATCurrentSector,fatbuf); // write the FAT buffer
   #ifdef FAT_DEBUG_RW_HITS
    FATWRHits++;
   #endif
   FATStatus=0;
  } 

 WriteFileSector(FileFirstClusterSector + FileClusterSectorOffset,iob); //write last sector used
#else
 if(iob_status==IOB_FAT && FATStatus>0)
  {
   WriteFATSector(FATCurrentSector,iob); // write the old FAT buffer
   #ifdef FAT_DEBUG_RW_HITS
    FATWRHits++;
   #endif
   FATStatus=0;
  }

 //if last access to iob was a FAT access than we should not write FileCurrentSector here
 if(iob_status==IOB_DATA)
  {
   WriteFileSector(FileFirstClusterSector + FileClusterSectorOffset,iob); //write last sector used
  }
#endif

 //update file entry filesize and date/time
 UpdateFileEntry();
 ReadFileSector(FileFirstClusterSector + FileClusterSectorOffset,iob); //iob was changed in UpdateFileEntry()
}
#endif //DOS_WRITE

//############################################################
/*!\brief Search for a filename in current directory
 * \param		name	8.3 DOS name
 * \return 		FULL_MATCH if file exists, NO_MATCH if not
 *
 */
U8 FindName(char *name)
//############################################################
{
 U8 result;

 if(FileFlag>0) return NO_MATCH; //don't search if a file is open
 
 result=ScanDirectory(name);

 return result;
}

#ifdef DOS_FSEEK
#ifdef DOS_READ
/*****************************************************************************/
// First version by Alex ??? 28.06.2007
/*!\brief Seek in a file
 * \param		offset	position to seek to (forward,backward)
 * \param		mode	start position from where to seek
 * \return 		0 if seeking successfull, >0 if not
 *
  Fseek() can (til now) only be used for files open for reading !
  You can not seek above FileSize.
  
  Parameters for mode:

  SEEK_SET	Seek from beginning of file. offset has to be positive.

  SEEK_CUR	Seek from current file pointer position. offset may be positive or negative.

  SEEK_END	Seek from end of file. offset has to be negative.
*/
/*****************************************************************************/
U8 Fseek(S32 offset, U8 mode)
{
#ifdef USE_FAT32
 U32 numClusters, curCluster;
#else
 U16 numClusters, curCluster;
#endif

 
 if(FileFlag==0) return 1; //don't seek if file is closed
 if(FileFlag==F_WRITE) return 2; //don't seek if file is open for writing

 // calculate the new File Position
 switch (mode)
  {
   case SEEK_SET: 	
        if(offset<0) return 3; 		// don't seek below 0
        if(offset > FileSize) return 4;   // don't seek above FileSize

        FilePosition = offset;           	
    break;

   case SEEK_END: // offset has to be 0 or negative	!
        if(offset>0) return 5; 		// don't seek above FileSize
        if((FileSize + offset) < 0) return 6; 		// don't seek below 0

        FilePosition = FileSize + offset; // offset is negativ !	
    break;

   case SEEK_CUR: 	
        if((FilePosition + offset) < 0) return 7;	// don't seek below 0
        if((FilePosition + offset) > FileSize) return 8;	// don't seek above FileSize

        FilePosition = FilePosition + offset;	
    break;

   default:    return 9;
  }
	
 // test if new FilePosition exists
// if(FilePosition > FileSize) return F_ERROR; // Notbremse
	
// numSector = (FilePosition / BYTE_PER_SEC) % secPerCluster;
// numSector = ((FilePosition >> 9) % secPerCluster);

 // secPerCluster is always power of 2 and <=128 !
 FileClusterSectorOffset = ((U8)(FilePosition >> 9) & (secPerCluster-1) );
 
// numClusters = FilePosition / (BYTE_PER_SEC*secPerCluster);
 numClusters = (FilePosition >> 9) / secPerCluster;

 // calculate the current file cluster
 curCluster = FileFirstCluster;
 FileCurrentCluster = curCluster;

 // calculate the cluster address of the new Position (verkettete pointerliste)
 FileClusterCount=0;
 while (numClusters > 0)
  {
   curCluster = GetNextClusterNumber(curCluster);
   if(curCluster < endofclusterchain)
    {
     FileCurrentCluster = curCluster;
     FileClusterCount += BytesPerCluster;
    }
   numClusters--;
  }
    

 // calculate the Sector address of the new Position
 FileFirstClusterSector = GetFirstSectorOfCluster(FileCurrentCluster);

 ReadFileSector(FileFirstClusterSector + FileClusterSectorOffset,iob); //read current sector of file
	
 return 0;
}
#endif //DOS_READ
#endif //DOS_FSEEK

#ifdef DOS_WRITE
 #ifdef DOS_RENAME
 
//###########################################################
/*!\brief Rename a file/dir
 * \param		OldName	DOS 8.3 name of the file/dir 
 * \param		NewName	DOS 8.3 name of the file/dir 
 * \return 		F_OK if file/dir could be renamed, F_ERROR if not
 */
U8 Rename(char *OldName, char *NewName)
//###########################################################
{
   if(FileFlag!=0) return F_ERROR; // don't rename if a file is open
  
   if(FindName(NewName)==FULL_MATCH) return F_ERROR; // File already exists !
   // FAT does NOT like DOUBLE filenames !
   
   FileAttr=0xFF; // test if we really find a file or a directory

   if(FindName(OldName)==FULL_MATCH)  //To fill some usefull variables...
    {
     if(FileAttr != ATTR_FILE && FileAttr != ATTR_DIRECTORY) return F_ERROR; // this was not a file/dir !

     MakeDirEntryName(NewName); //Split into name and extension
     UpdateFileEntry();

     return F_OK; 
    }

 return F_ERROR; // OldName not found
}
 #endif
#endif

//@}

