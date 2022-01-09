//###########################################################
// File: readraw.c
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 15.10.2006
//#########################################################################
// hk@holger-klabunde.de
// http://www.holger-klabunde.de/index.html
//#########################################################################
// Compiler: avr-gcc 3.4.5
//#########################################################################
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dos.h"

#ifdef DOS_READ_RAW
//###########################################################
// reads all clusters/sectors of the file.
// you have to check FileSize from direntry to see how
// many bytes of last cluster are valid for the file. 
unsigned char ReadFileRaw(char *name)
//###########################################################
{
 unsigned long tmpsector;

#ifdef USE_FAT32
 unsigned long tmp;
#else
 unsigned int tmp;
#endif

 unsigned char *p;
 unsigned char by,secloop;
 unsigned int i;
 unsigned char end_of_file_is_near;

 if(FileFlag>0) return F_ERROR; // there is an open file !

 if(FindName(name)==FULL_MATCH)
  {
   tmp=FileFirstCluster;
   FilePosition=0; //actual read position
   FileFlag=1;

   while(tmp < endofclusterchain) // read all clusters of file
    {
     tmpsector=GetFirstSectorOfCluster(tmp);

     // read all sectors of this cluster
     for(secloop=0; secloop<secPerCluster; secloop++)
      {
       ReadFileSector(tmpsector,iob);
       #ifndef USE_FATBUFFER
        iob_status=IOB_DATA;
       #endif

       p=iob;          //pointer to io buffer

       if((FileSize-FilePosition) > BYTE_PER_SEC) end_of_file_is_near = 0;
       else end_of_file_is_near=1;

       // read data from iob now
       for(i=0; i<BYTE_PER_SEC; i++) // read all bytes of a sector
        {
         by = *p++; // get a byte from iob
         // 'by' holds your data now. use it as you want 
         // put your function to use 'by' below

         //****************************
         // load your MP3-Chip here ;)
         //****************************

         // end of your function
         
         if(end_of_file_is_near)
          {
           FilePosition++; // count bytes read
           if(FilePosition==FileSize)
            {
             FileFlag=0;
             return F_OK; // end of file reached ?
            } 
          } 
        } 

       FilePosition += BYTE_PER_SEC;
       
       tmpsector++; // next sector of cluster
      }// for(secloop=0; secloop<secPerCluster; secloop++)

     tmp=GetNextClusterNumber(tmp);
    }// while(tmp < endofclusterchain)

  }//if(FindName(name)==FULL_MATCH)
 else return F_ERROR;  // something went wrong

 FileFlag=0;

 return F_OK;
}
#endif
