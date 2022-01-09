/*! \file dir.c
    \brief Directory-Functions
*/
//###########################################################
///	\ingroup singlefat
///	\defgroup DIR DIR-Functions (dir.c)
///	\code #include "dir.h" \endcode
///	\code #include "dos.h" \endcode
///	\par Uebersicht
//###########################################################
//
// For FAT12, FAT16 and FAT32
// Only for first Partition
// Only for drives with 512 bytes per sector (the most)
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
// 24.08.2007 Changed MakeFileName() to MakeDirEntryName().
//            A lot of optimizations to make smaller code.
//
// 10.08.2007 A lot of changes in ScanOneDirectorySector().
//
// 08.08.2007 Merged ScanRootDir() and ScanSubDir() in one function ScanDirectory()
//
// 20.11.2006 Bugfix. If WIN makes a directory for FAT32 and directory
//            is in Root directory, upperdir cluster is ZERO !
//
//#########################################################################
// Last change: 24.08.2007
//#########################################################################
// hk@holger-klabunde.de
// http://www.holger-klabunde.de/index.html
//#########################################################################
// Compiler: AVR-GCC 4.1.1
//#########################################################################
//@{
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dos.h"

#ifdef USE_FAT32
 U32 FirstDirCluster=0;
#else
 U16 FirstDirCluster=0;
#endif

#ifdef DOS_WRITE
//###########################################################
/*!\brief Make DOS time from system time
 * \return 		time in DOS format 
 *
 * If your system has an RTC get your time here and make a DOS time
 */
static inline U16 DOSTime(void)
//###########################################################
{
 U16 time;

// while(seconds==59); // maybe we should do this !?
 
// Get your system time here
// fixed time 16:19:33 if no clock exists 
 time  = (U16)16 << 11; // hours
 time |= (U16)19 << 5;  // minutes
 time |= 33 / 2;        // seconds

 return time;
}
#endif //DOS_WRITE

#ifdef DOS_WRITE
//###########################################################
/*!\brief Make DOS date from system date
 * \return 		date in DOS format 
 *
 * If your system has an RTC get your date here and make a DOS date
 */
static inline U16 DOSDate(void)
//###########################################################
{
 U16 date;
 
// Get your system date here
// fixed date 02.08.2007 if no clock exists 
 date = 2007 - 1980;  // years since 1980
 date <<= 9;
 
 date |= (U16)8 << 5;  // month
 date |= (U16)2;      // day

 return date;
}
#endif //DOS_WRITE

#ifdef DOS_WRITE
//###########################################################
/*!\brief Search dir for free dir entry
 * \return 		Above 0 if successfull, 0 if not
 */
#ifdef USE_FAT32
 static inline U8 SearchFreeDirentry(void)
#else
 static inline U8 SearchFreeDirentry(void)
#endif
//###########################################################
{
 U32 tmpsector;

#ifdef USE_FAT32
 U32 tmpcluster;
#else
 U16 tmpcluster;
#endif

 U8 i,result;
 result=0;
 
 if(FirstDirCluster<2) // We are in rootdir for FAT12/16
  {
   tmpsector = FirstRootSector;
   
   i = RootDirSectors;
   do
    {
     result=SearchDirSector(tmpsector++);
     if(result!=0) break; //break sector loop
     i--;
    }while(i);
  } 
 else  // We are in a subdir
  {
   tmpcluster=FirstDirCluster;
 
   while(tmpcluster < endofclusterchain)
    {
     tmpsector=GetFirstSectorOfCluster(tmpcluster);

     i = secPerCluster;
     do
      {
       result=SearchDirSector(tmpsector++);
       if(result!=0) break; //break sector loop
       i--;
      }while(i); 

     if(result!=0) break; //break cluster loop
     tmpcluster=GetNextClusterNumber(tmpcluster);
    }
  }

 return result;
}
#endif //DOS_WRITE

#ifdef DOS_WRITE
//###########################################################
/*!\brief Search dir sector for free dir entry
 * \param		sector	Sector number of the directory
 * \return 		Above 0 if successfull, 0 if not
 */
U8 SearchDirSector(U32 sector)
//###########################################################
{
 U16 count;

 ReadDirSector(sector,iob); //read one directory sector.
 count=0;
 do
  {
   if(iob[count]==0 || iob[count]==0xE5)
    {
     FileDirSector=sector; //keep some values in mind
     FileDirOffset=count/32;
     return 1;
    }

   count+=32;
  }while(count<BYTE_PER_SEC);

 return 0;
}
#endif //DOS_WRITE

#ifdef DOS_WRITE
//###########################################################
/*!\brief Fill a cluster with zeros
 * \param		startsector
 * \return 		nothing
 */
void ZeroCluster(U32 startsector)
//###########################################################
{
 U32 sector;
 U16 i;

 sector = startsector;

 //clean all sectors of a cluster (fill with 0x00)
 for(i=0; i<BYTE_PER_SEC; i++) iob[i]=0; //Fill write buffer

 unsigned char j = secPerCluster;

 do
  {
   WriteDirSector(sector++,iob);
   j--;
  }while(j); 

}
#endif //DOS_WRITE

#ifdef DOS_WRITE
#ifdef DOS_MKDIR
//###########################################################
/*!\brief Make a new directory
 * \param		name	8.3 DOS name
 * \return 		F_OK if successfull, otherwise F_ERROR
 */
U8 Mkdir(char *name)
//###########################################################
{
 if(FileFlag!=0) return F_ERROR; // no open file allowed here

 //Test if directoryname exists in current directory
 if(FindName(name)==FULL_MATCH)
  {
   if(FileAttr == ATTR_DIRECTORY) return F_OK; // Directory already exists
   return F_ERROR; // There is a FILE named "name" !
  }

//from this point i use some variables of FILE entry !
//you have to change a lot of things if you want to use more then ONE file !

 FileAttr=ATTR_DIRECTORY;  //want to make a directory !
 MakeDirEntryName(name);
 if(MakeNewFileEntry())
  {
   //MakeNewFileEntry() allocates first cluster for new directory !
   //First cluster of new directory is returned in FileFirstCluster

   FileDirSector=GetFirstSectorOfCluster(FileFirstCluster);

   ZeroCluster(FileDirSector);

   //insert "." my new dir entry with newdir firstcluster
   FileDirOffset=0; //first direntry "."
   MakeDirEntryName(".");
   UpdateFileEntry();

   //insert ".." upper dir entry with upperdir firstcluster
   FileDirOffset=1;  //2nd direntry ".."
   MakeDirEntryName("..");
   FileFirstCluster=FirstDirCluster;
   UpdateFileEntry();

#ifdef USE_FATBUFFER
     WriteSector(FATCurrentSector,fatbuf); // write the FAT buffer
     FATStatus=0;
#endif
  }
 else
  {
   FileAttr=ATTR_FILE; //default
   return F_ERROR; //new dir could not be made
  } 
 
 FileAttr=ATTR_FILE; //default
 return F_OK;
}
#endif //DOS_MKDIR
#endif //DOS_WRITE
       
#ifdef DOS_CHDIR
//###########################################################
/*!\brief Change to a directory
 * \param		name	8.3 DOS name
 * \return 		F_OK if successfull, otherwise F_ERROR
 */
U8 Chdir(char *name)
//###########################################################
{
 if(FileFlag!=0) return F_ERROR; // no open file allowed here

 if(name[0]=='/')
  {
#ifdef USE_FAT12
   if(FATtype==FAT12) FirstDirCluster=0;
#endif
#ifdef USE_FAT16
   if(FATtype==FAT16) FirstDirCluster=0;
#endif
#ifdef USE_FAT32
   if(FATtype==FAT32) FirstDirCluster=FAT32RootCluster;
#endif
   return F_OK;
  }
 
 if(FindName(name)==FULL_MATCH)
  {
   if(FileAttr == ATTR_DIRECTORY)
    {
     //First cluster of directory is returned in FileFirstCluster
     FirstDirCluster = FileFirstCluster;
     return F_OK;
    } 
  }
  
 return F_ERROR; // Maybe there is a FILE named "name" !
}
#endif //DOS_CHDIR

#ifdef DOS_WRITE
//###########################################################
/*!\brief Make a new file/directory entry in current directory
 * \return 		F_OK if successfull, otherwise F_ERROR
 */
U8 MakeNewFileEntry(void)
//###########################################################
{
#ifdef USE_FAT32
 U32 tmpcluster,lastdircluster;
#else
 U16 tmpcluster,lastdircluster;
#endif

 U8 result;
 
 result=SearchFreeDirentry();
 if(result==0) // no free direntry found. lets try to alloc and add a new dircluster
  {
   //if dir is rootdir (FAT12/16 only) you have a problem ;)
#if defined (USE_FAT12) || defined (USE_FAT16)
   if(FirstDirCluster<2)
    {
#ifdef USE_FAT12
     if(FATtype==FAT12) return F_ERROR;
#endif
#ifdef USE_FAT16
     if(FATtype==FAT16) return F_ERROR;
#endif
    }
#endif

   //search the last cluster of directory
   lastdircluster=FirstDirCluster;
   do
    {
     tmpcluster=GetNextClusterNumber(lastdircluster);
     if(tmpcluster < endofclusterchain) lastdircluster=tmpcluster;
    }while(tmpcluster < endofclusterchain);
   
   tmpcluster=AllocCluster(lastdircluster); //if current directory is full alloc new cluster for dir
   if(tmpcluster==DISK_FULL) //no free clusters ?
    {
     return F_ERROR;
    }

   FileDirSector=GetFirstSectorOfCluster(tmpcluster);

   ZeroCluster(FileDirSector);

   FileDirOffset=0;              //set offset for new direntry in dirsector
  }     

 tmpcluster=AllocCluster(0); //alloc first cluster for file
 if(tmpcluster==DISK_FULL) //no free clusters ?
  {
   return F_ERROR;
  }

 FileFirstCluster=tmpcluster;
 FileSize=0;
 UpdateFileEntry(); //write new file entry

 return F_OK; //all ok 
}
#endif //DOS_WRITE

#ifdef DOS_WRITE
//###########################################################
/*!\brief Update directory entry of a file/dir
 * \return 		F_OK if successfull, F_ERROR if not
 */
U8 UpdateFileEntry(void)
//###########################################################
{
 struct DirEntry *di;
 
 ReadDirSector(FileDirSector,iob);
 di=(struct DirEntry *)&iob[FileDirOffset*32];

 strncpy(di->DIR_Name,FileName,11);

 di->DIR_Attr=FileAttr;
 di->DIR_NTres=0;
 di->DIR_CrtTimeTenth=0;

 di->DIR_CrtTime = DOSTime();           //creation time
 di->DIR_WrtTime = di->DIR_CrtTime;     //last write time
 di->DIR_CrtDate = DOSDate();           //creation date
 di->DIR_WrtDate = di->DIR_CrtDate;     //last write date
 di->DIR_LastAccDate = di->DIR_CrtDate; //last access date

#ifdef USE_FAT32
 di->DIR_FstClusHI=(U16)(FileFirstCluster>>16);  //first cluster high word                 
#else
 di->DIR_FstClusHI=0;  //first cluster high word                 
#endif
 di->DIR_FstClusLO=(U16)(FileFirstCluster);  //first cluster low word                 
 di->DIR_FileSize=FileSize;

 WriteDirSector(FileDirSector,iob);
 return F_OK;
}
#endif //DOS_WRITE

//###########################################################
/*!\brief Heart of the directory functions
 * \param		sector	Sector number of the directory
 * \param		name	name of a file/dir to search for or NULL for Findfirst(), Findnext()
 * \return 		FULL_MATCH if successfull, NO_MATCH or END_DIR if not
 *

 Following global variables will be set if dir or filename was found:

 For directories:

 ================

 FileFirstCluster  First cluster of directory

 FileName          8 chars for dirname

 FileExt           3 chars for extension

 FileSize          = 0

 FileAttr          = ATTR_DIRECTORY

 For files:

 ==========

 FileName          8 chars for filename

 FileExt           3 chars for extension

 FileDirSector     Sector which keeps direntry of the file

 FileDirOffset     Offset to direntry of the file

 FileFirstCluster  First cluster of file in FAT 

 FileSize

 FileAttr          = ATTR_FILE

 */
U8 ScanOneDirectorySector(U32 sector, char *name)
//###########################################################
{
 U8 by;
 U16 count;
 struct DirEntry *di;
 struct DirEntryBuffer *dib;
 U8 match;

 if(name) //find filename or directory name
  {
   MakeDirEntryName(name);
  }
 
 by=ReadDirSector(sector,iob); //read one directory sector.
 count=0;
 do
  {
   match=NO_MATCH;
   di=(struct DirEntry *)(&iob[count]);

   //make a second pointer to iob for easier access to long filename entrys
   dib=(struct DirEntryBuffer *)di;
   
   if(di->DIR_Name[0]==0) return END_DIR; //end of directory
      
   if((unsigned char)di->DIR_Name[0]!=0xE5) // Deleted entry ?
    {
     di->DIR_Attr&=0x3F;            //smash upper two bits

     if(di->DIR_Attr==ATTR_LONG_NAME)
      {
#ifdef USE_FINDFILE
#ifdef USE_FINDLONG
       if(name==NULL) // FIND_OPERATION
        {
         // Build the long name from the 13 bytes long name direntrys.
         // The long name direntrys have to be in a block of direntrys.
         // Otherwise this will not work and you get strange results.
       
         if(ffblk.newposition >= ffblk.lastposition) //found a new file ?
          {
           U8 offset; //offset of the direntry in long name
           U8 ffcount;  //loop counter
           U8 i;

           offset=dib->longchars[0];
           offset&=0x1F; //force upper bits D7..5 to zero.
                         //max. value of 20 is allowed here, or even less
                         //if _MAX_NAME is defined smaller than 255.
                         //long filenames will then be cut at _MAX_NAME - 1.

           if(offset>20) offset=20; //force maximum value if too big

           ffcount=(offset-1) * 13; // calc start adress in long name array
       
           //We can not use strncpy() because a CHAR in the long name
           //direntry has two bytes ! 2nd byte is ignored here.
       
           for(i=1; i<=9; i+=2)
            {
             by=dib->longchars[i];
             if(ffcount<_MAX_NAME) ffblk.ff_longname[ffcount]=by;
             ffcount++;
            }

           for(i=14; i<=24; i+=2)
            {
             by=dib->longchars[i];
             if(ffcount<_MAX_NAME) ffblk.ff_longname[ffcount]=by;
             ffcount++;
            }

           for(i=28; i<=30; i+=2)
            {
             by=dib->longchars[i];
             if(ffcount<_MAX_NAME) ffblk.ff_longname[ffcount]=by;
             ffcount++;
            }

           ffblk.ff_longname[_MAX_NAME-1]=0; //End of string to avoid buffer overruns 

          }//if(ffblk.newposition >= ffblk.lastposition)
        } //if(name==NULL)
#endif //#ifdef USE_FINDLONG
#endif //#ifdef USE_FINDFILE       
      }
     else  //no long name entry
      {
       if(name!=NULL) { if(strncmp(FileName,di->DIR_Name,11)==0) match=FULL_MATCH; }
                
       if(di->DIR_Attr & ATTR_VOLUME_ID)
        {         //nothing to do here. volume id not supported
        }
       else
        {
         // If we come here we have a file or a directory
         if(name==NULL)  //FIND_OPERATION
          {
#ifdef USE_FINDFILE
           ffblk.newposition++; //one more entry found
             
           if(ffblk.newposition > ffblk.lastposition) //found a new file ?
            {
             ffblk.lastposition=ffblk.newposition;    //save new file position

             // di->DIR_Name may look like this "0123     ext"
             // But we need this "0123.ext". If there is no extension
             // we also don't want a '.'.
             
             unsigned char pos,pos1;
             
             pos = 0;
             do
              {
               if(di->DIR_Name[pos] == ' ') break;
               else ffblk.ff_name[pos] = di->DIR_Name[pos];
               pos++;
              }while(pos < 8);
             
             if(di->DIR_Name[8] != ' ') // we have an extension
              {
               ffblk.ff_name[pos++] = '.'; // insert '.'

               pos1 = 8; // position of extension
               do
                {
                 if(di->DIR_Name[pos1] == ' ') break;
                 else ffblk.ff_name[pos++] = di->DIR_Name[pos1];
                 pos1++;
                }while(pos1 < 11);
               
              }  

//             strncpy(ffblk.ff_name,di->DIR_Name,8);   //copy filename
//             strncpy(&ffblk.ff_name[9],&di->DIR_Name[8],3);//copy fileextension
             
             if(di->DIR_Attr & ATTR_DIRECTORY)
              {
               ffblk.ff_attr=ATTR_DIRECTORY;      //file attribute
               ffblk.ff_fsize=0; 		  //not a file, clear filesize
              }
             else
              {
               ffblk.ff_attr=ATTR_FILE;
               ffblk.ff_fsize=di->DIR_FileSize;
              }

             return FULL_MATCH; //found next entry, stop searching

            } //if(ffblk.newposition > ffblk.lastposition)
#endif//#ifdef USE_FINDFILE
          } //if(name==NULL)
         else //FILE/DIR operation
          { 
#ifdef USE_FAT32
           FileFirstCluster = di->DIR_FstClusHI; //Highword of first cluster number
           FileFirstCluster <<=16;
           FileFirstCluster += di->DIR_FstClusLO; //Lowword of first cluster number
#else
           FileFirstCluster = di->DIR_FstClusLO; //Lowword of first cluster number
#endif

           FileDirSector=sector; //keep some values in mind
           FileDirOffset=count/32;

           if(match==FULL_MATCH)
            {

             if(di->DIR_Attr & ATTR_DIRECTORY) //this is a directory
              {

#ifdef USE_FAT32
// Special case for FAT32 and directories in ROOT directory MADE BY WIN.
// Upper directory ".." first cluster for a subdirectory is ZERO here, and NOT FAT32RootCluster !
// Bug found by Andreas ???

               if(FATtype==FAT32)
                {
                 if(FileFirstCluster < 2) FileFirstCluster = FAT32RootCluster; // force to correct cluster
                }
#endif //#ifdef USE_FAT32

               FileSize = 0; // Directorys have no size
               FileAttr = ATTR_DIRECTORY;

              }//if(di->DIR_Attr & ATTR_DIRECTORY)
             else //is not a directory. this is a file
              {
               FileSize=di->DIR_FileSize;
               FileAttr=ATTR_FILE;
              }

             return FULL_MATCH;

            }//if(match==FULL_MATCH) 
          } //else if(name==NULL)
        }//if(di->DIR_Attr & ATTR_VOLUME_ID)
      }
         
    }//if(di->DIR_Name[0]!=0xE5)
        
   count+=32;
  }while(count<BYTE_PER_SEC);

 return NO_MATCH;
}


//###########################################################
/*!\brief Scan directory for files or directorys
 * \param               name 8.3 DOS name
 * \return 		FULL_MATCH if successfull, NO_MATCH if not
 */
#ifdef USE_FAT32
 U8 ScanDirectory(char *name)
#else
 U8 ScanDirectory(char *name)
#endif
//###########################################################
{
 U32 tmpsector;

#ifdef USE_FAT32
 U32 tmpcluster;
#else
 U16 tmpcluster;
#endif

 U8 i,result;
 
 result=NO_MATCH;
 
 if(FirstDirCluster < 2) // Is this a FAT12/FAT16 rootdirectory ?
  {
   tmpsector = FirstRootSector;

   i = RootDirSectors;
   do
    {
     result=ScanOneDirectorySector(tmpsector++, name);
     if(result!=NO_MATCH) break; //break sector loop
     i--;
    }while(i);
  }
 else // We are in a subdirectory 
  {
   tmpcluster=FirstDirCluster;

   while(tmpcluster < endofclusterchain)
    {
     tmpsector=GetFirstSectorOfCluster(tmpcluster);

     i = secPerCluster;
     do
      {
       result=ScanOneDirectorySector(tmpsector++, name);
       if(result!=NO_MATCH) break; //break sector loop
       i--;
      }while(i); 

     if(result!=NO_MATCH) break; //break cluster loop
     tmpcluster=GetNextClusterNumber(tmpcluster);
    }
  } 

 return(result);
}

//###########################################################
/*!\brief Change a 8.3 DOS name to a direntry formatted name
 * \param		inname	8.3 DOS name as a string ("test.txt")
 * \return 		FileName global
 */
void MakeDirEntryName(char *inname)
//###########################################################
{
 U8 by,i,j;
 char *po;

 po=FileName;
 i=11;
 while(i--) *po++ = ' '; //fill filename buffer with spaces

 po=FileName;

 if(inname[0]=='.')
  {
   *po++ ='.';
   if(inname[1]=='.') *po ='.'; //change to upper dir

   return;
  }

 i=0; //get filename and make it uppercase
 j=0;
 do
  {
   by=inname[i++];
   if(by == 0) return;             // end of filename reached
   if(by == '.') po = &FileName[8]; // extension reached
   else
    {
     *po++ = toupper(by);
     j++;
    } 
  }while(j<11);

// for(i=0; i<11; i++) putchar(FileName[i]);
}

//@}


