/*! \file fat.c \brief FAT-Functions */
//###########################################################
///	\ingroup singlefat
///	\defgroup FAT FAT-Functions (fat.c)
///	\code #include "fat.h" \endcode
///	\code #include "dos.h" \endcode
///	\par Uebersicht
//###########################################################
// For FAT12, FAT16 and FAT32
// Only for first Partition
// Only for drives with 512 bytes per sector (the most)
//
// Based on a White Paper from MS
// FAT: General Overview of On-Disk Format
// Version 1.03, December 6, 2000
//
// MBR MasterBootRecord
// PC intern 4
// M.Tischer
// Data Becker
//
// 17.09.2007 Removed most #ifdef USE_FATBUFFER. See dos.h.
//            Sourcecode looks much better now.
//
// 11.10.2006 Replaced "% BYTE_PER_SEC" with "& (BYTE_PER_SEC-1)".
//            Typecast variables from "unsigned long" to "unsigned int" before:
//            secoffset = (unsigned int)fatoffset & (BYTE_PER_SEC-1);
//            Use "unsigned int" for indexing arrays. Does not really speed up,
//            but reduces code size ;)
//
// 25.09.2006 Initialize all global variables to zero for better compatibility
//            with other compilers.
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 17.09.2007
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

#if defined (FAT_DEBUG_SHOW_FAT_INFO) || defined (FAT_DEBUG_RW_HITS) || defined (FAT_DEBUG_CLUSTERS)
 #include "serial.h" //for testing only
 #include "printf.h" //for testing only
#endif

U8 iob[BYTE_PER_SEC];      //file i/o buffer

U32 FirstDataSector=0; 
U32 FirstRootSector=0; 
U32 FATFirstSector=0;  
U8 FATtype=0;         

#ifdef USE_FAT32
 U32 FAT32RootCluster=0;
 U32 endofclusterchain=0; //value for END_OF_CLUSTERCHAIN 
 U32 maxcluster=0;	 // last usable cluster+1

 #ifdef FAT_DEBUG_RW_HITS
  U32 FATWRHits=0;
  U32 FATRDHits=0;
 #endif

#else
 U16 endofclusterchain=0; //value for END_OF_CLUSTERCHAIN 
 U16 maxcluster=0;	 // last usable cluster+1

 #ifdef FAT_DEBUG_RW_HITS
  U16 FATWRHits=0;
  U16 FATRDHits=0;
 #endif
#endif

U8 iob_status=0;
U32 FATCurrentSector=0;

#ifdef DOS_WRITE
  U8 FATStatus=0; // only for FAT write buffering
#endif

#ifdef USE_FATBUFFER
 U8 fatbuf[BYTE_PER_SEC];   //buffer for FAT sectors
#endif

U8 secPerCluster=0;

#ifdef USE_64k_CLUSTERS
 U32 BytesPerCluster=0;   //bytes per cluster
#else
 U16 BytesPerCluster=0;   //bytes per cluster
#endif

//U32 RootDirSectors=0; // to big !
//U16 RootDirSectors=0;   // maybe U8 is enough.
U8 RootDirSectors=0;   // never saw more then 32 sectors


//############################################################
/*!\brief Decide if we have to write a used fat sector or read a new fat sector
 * \param		newsector Actual sector number
 * \return 		Nothing
 */
void UpdateFATBuffer(U32 newsector)
//############################################################
{
#ifdef USE_FATBUFFER
#else
 if(iob_status!=IOB_FAT) // We have to read a FAT sector first if this is true
 {
   ReadFATSector(newsector,iob); //read FAT sector
   #ifdef FAT_DEBUG_RW_HITS
    FATRDHits++;
   #endif
   FATCurrentSector=newsector;
 }
#endif //#ifdef USE_FATBUFFER

 if(newsector!=FATCurrentSector) // do we need to update the FAT buffer ?
  {
#ifdef DOS_WRITE
   if(FATStatus>0)
    {
     WriteFATSector(FATCurrentSector,FAT_IO_BUFFER); // write the old FAT buffer
     #ifdef FAT_DEBUG_RW_HITS
      FATWRHits++;
     #endif
     FATStatus=0; // flag FAT buffer is save
    } 
#endif
   ReadFATSector(newsector,FAT_IO_BUFFER); //read FAT sector
   #ifdef FAT_DEBUG_RW_HITS
    FATRDHits++;
   #endif

   FATCurrentSector=newsector;
  } 
}

//############################################################
/*!\brief Get back next cluster number from fat cluster chain
 * \param		cluster	Actual cluster number
 * \return 		Next cluster number
 */
#ifdef USE_FAT32
 U32 GetNextClusterNumber(U32 cluster)
#else
 U16 GetNextClusterNumber(U16 cluster)
#endif
//############################################################
{
#ifdef USE_FAT12
 U16 tmp, secoffset;
 U8 fatoffset;
#endif

 union Convert *cv;

#ifdef FAT_DEBUG_CLUSTERS
 #ifdef USE_FAT32
  printf("GNCN %lu\n",cluster);
 #else
  printf("GNCN %u\n",cluster);
 #endif
#endif

 if(cluster<maxcluster) //we need to check this ;-)
  {

#ifdef USE_FAT12
   if(FATtype==FAT12)
    {
     // FAT12 has 1.5 Bytes per FAT entry
     // FAT12 can only have 4085 clusters. So cluster * 3 is 16 bit 
     tmp = ((U16)cluster * 3) >>1 ; //multiply by 1.5 (rounds down)
     secoffset = (U16)tmp & (BYTE_PER_SEC-1); //we need this for later

     // FAT12 4085 Cluster * 1.5Bytes = 6127.5 Bytes => max 12 FAT sectors
     // FAT sector offset is 8 Bit 
//     fatoffset = (U8)(tmp / BYTE_PER_SEC); //sector offset from FATFirstSector
     fatoffset = (U8)(tmp >> 9); //sector offset from FATFirstSector
 
     UpdateFATBuffer(FATFirstSector + fatoffset); //read FAT sector

     if(secoffset == (BYTE_PER_SEC-1)) //if this is the case, cluster number is
                                   //on a sector boundary. read the next sector too
      {
       tmp = FAT_IO_BUFFER[BYTE_PER_SEC-1]; //keep first part of cluster number
       UpdateFATBuffer(FATFirstSector + fatoffset +1 ); //read next FAT sector
       tmp += (U16)FAT_IO_BUFFER[0] << 8; //second part of cluster number
      }
     else
      {
       cv=(union Convert *)&FAT_IO_BUFFER[secoffset];
       tmp=cv->ui;
      } 

     if((U8)cluster & 0x01) tmp>>=4; //shift to right position
     else               tmp&=0xFFF; //delete high nibble

     return (tmp);
    }//if(FATtype==FAT12)
#endif //#ifdef USE_FAT12

#ifdef USE_FAT16
   if(FATtype==FAT16)
    {
     //two bytes per FAT entry
//     UpdateFATBuffer(FATFirstSector + ((U32)cluster * 2) / BYTE_PER_SEC);
//     UpdateFATBuffer(FATFirstSector + ((unsigned int)cluster) / (BYTE_PER_SEC/2));

     UpdateFATBuffer(FATFirstSector + (U8)( (U16)cluster >> 8 ));

     // Buffer index is max 511. So in any case we loose upper bits. typecast cluster to U16
//     cv=(union Convert *)&fatbuf[((U32)cluster * 2) % BYTE_PER_SEC];
     cv=(union Convert *)&FAT_IO_BUFFER[((U16)cluster << 1) & (BYTE_PER_SEC-1)];
     return(cv->ui);
    }//if(FATtype==FAT16)
#endif //#ifdef USE_FAT16

#ifdef USE_FAT32
   if(FATtype==FAT32)
    {
     //four bytes per FAT entry
//     UpdateFATBuffer(FATFirstSector + (cluster * 4) / BYTE_PER_SEC);
//     UpdateFATBuffer(FATFirstSector + cluster / (BYTE_PER_SEC/4));

     UpdateFATBuffer(FATFirstSector + ( cluster >> 7 ));

     // Buffer index is max 511. So in any case we loose upper bits. typecast cluster to U16
     cv=(union Convert *)&FAT_IO_BUFFER[((U16)cluster << 2) & (BYTE_PER_SEC-1)];
     return( cv->ul & 0x0FFFFFFF );
    }//if(FATtype==FAT32) 
#endif //#ifdef USE_FAT32
  }
  
 return DISK_FULL; //return impossible cluster number
}

//###########################################################
/*!\brief Get back number of first sector of cluster
 * \param		cluster	Actual cluster number
 * \return 		Sector number
 */
#ifdef USE_FAT32
 U32 GetFirstSectorOfCluster(U32 cluster)
#else
 U32 GetFirstSectorOfCluster(U16 cluster)
#endif
//###########################################################
{
// Komische Sache: Die Schieberei hier bringt bei ATmega32 ca. 200 Byte weniger Code.
// Bei ATmega644 wird der Code dadurch ca. 20 Bytes größer !

 U8 temp;
 U32 templong;
 
 templong = cluster-2;

 // secPerCluster is always power of two 
 temp = secPerCluster>>1; // don't multiply with 1 ;)
 
 while(temp)
  {
   templong <<= 1;
   temp >>= 1;
  }
 
 return (templong + FirstDataSector);

// return (((U32)(cluster - 2) * secPerCluster) + FirstDataSector);
}

#ifdef DOS_WRITE
//###########################################################
/*!\brief Allocate a new cluster in cluster chain
 * \param		currentcluster	Actual cluster number
 * \return 		New cluster number or DISK_FULL
 */
#ifdef USE_FAT32
 U32 AllocCluster(U32 currentcluster)
#else
 U16 AllocCluster(U16 currentcluster)
#endif
//###########################################################
{
#ifdef USE_FAT32
 U32 cluster;
#else
 U16 cluster;
#endif

// do this if you want to search from beginning of FAT
// cluster=FindFreeCluster(0); //get next free cluster number
 cluster=FindFreeCluster(currentcluster); // get next free cluster number
 if(cluster!=DISK_FULL && cluster<=maxcluster) // disk full ?
  {
    // insert new cluster number into chain
    // currentcluster=0 means: this is a new cluster chain
    if(currentcluster>0) WriteClusterNumber(currentcluster,cluster);

   // mark end of cluster chain
#ifdef USE_FAT12
   if(FATtype==FAT12) WriteClusterNumber(cluster,0xFFF); 
#endif
#ifdef USE_FAT16
   if(FATtype==FAT16) WriteClusterNumber(cluster,0xFFFF); 
#endif
#ifdef USE_FAT32
   if(FATtype==FAT32) WriteClusterNumber(cluster,0x0FFFFFFF); 
#endif
  }

#ifdef USE_FATBUFFER
#else
   // We should flush the temporary fatbuffer now because next action is
   // filling iob[] with new data !
   if(FATStatus>0)
    {
     WriteFATSector(FATCurrentSector,iob); // write the FAT buffer
     #ifdef FAT_DEBUG_RW_HITS
      FATWRHits++;
     #endif
     FATStatus=0; // flag FAT buffer is save
    } 
#endif

 return cluster;
}
#endif //DOS_WRITE

#ifdef DOS_WRITE
//###########################################################
/*!\brief Find a free cluster in FAT
 * \param		currentcluster	Actual cluster number
 * \return 		Number of a free cluster or DISK_FULL
 */
#ifdef USE_FAT32
 U32 FindFreeCluster(U32 currentcluster)
#else
 U16 FindFreeCluster(U16 currentcluster)
#endif
//###########################################################
{
#ifdef USE_FAT32
 U32 cluster;
#else
 U16 cluster;
#endif

 cluster=currentcluster+1; // its a good idea to look here first
                           // maybe we do not need to search the whole FAT
                           // and can speed up free cluster search
                           // if you do not want this call FindFreeCluster(0)
// search til end of FAT
 while(cluster<maxcluster)
  {
   if(GetNextClusterNumber(cluster)==0) break;
   cluster++;
  }

// if we have not found a free cluster til end of FAT
// lets start a new search at beginning of FAT
 if(cluster>=maxcluster)
  {
   cluster=2; // first possible free cluster
   while(cluster<=currentcluster) // search til we come to where we have started
    {
     if(GetNextClusterNumber(cluster)==0) break;
     cluster++;
    }

   if(cluster>=currentcluster) return DISK_FULL; // no free cluster found
  }
  
 if(cluster>=maxcluster) return DISK_FULL;
    
 return cluster;
}
#endif //DOS_WRITE

#ifdef DOS_WRITE
//############################################################
/*!\brief Insert a new cluster number into cluster chain
 * \param		cluster	Actual cluster number
 * \param		number	Cluster number to append to cluster chain
 * \return 		Nothing til now (0)
 */
#ifdef USE_FAT32
 U8 WriteClusterNumber(U32 cluster, U32 number)
#else
 U8 WriteClusterNumber(U16 cluster, U16 number)
#endif
//############################################################
{
#ifdef USE_FAT12
 U16 tmp, secoffset;
 U8 fatoffset;
 U8 lo,hi;
 U32 sector;
#endif

 U8 *p;
 
#ifdef FAT_DEBUG_CLUSTERS
#ifdef USE_FAT32
 printf("WCN %lu\n",cluster);
#else
 printf("WCN %u\n",cluster);
#endif
#endif

 if(cluster<maxcluster) //we need to check this ;-)
  {

#ifdef USE_FAT12
   if(FATtype==FAT12)
    {
     //FAT12 has 1.5 Bytes per FAT entry
     // FAT12 can only have 4085 clusters. So cluster * 3 is 16 bit 
     tmp= ((U16)cluster * 3) >>1 ; //multiply by 1.5 (rounds down)
     secoffset = (U16)tmp & (BYTE_PER_SEC-1); //we need this for later
     // FAT12 4085 Cluster * 1.5Bytes = 6127.5 Bytes => max 12 FAT sectors
     // FAT sector offset is 8 Bit 
//     fatoffset = (U8)(tmp / BYTE_PER_SEC); //sector offset from FATFirstSector
     fatoffset = (U8)(tmp >> 9); //sector offset from FATFirstSector
     sector=FATFirstSector + fatoffset;

     tmp=(U16)number;
     if((U8)cluster & 0x01) tmp<<=4; //shift to right position
     lo=(U8)tmp;
     hi=(U8)(tmp>>8);
     
     UpdateFATBuffer(sector); //read FAT sector

     if(secoffset == (BYTE_PER_SEC-1)) //if this is the case, cluster number is
                                   //on a sector boundary. read the next sector too
      {

       p = &FAT_IO_BUFFER[BYTE_PER_SEC-1]; //keep first part of cluster number

       if((U8)cluster & 0x01)
        {
         *p&=0x0F;
         *p|=lo;
        }
       else *p=lo;
        
       FATStatus=1; // we have made an entry, so write before next FAT sector read
       UpdateFATBuffer(sector+1); //read FAT sector

       p = &FAT_IO_BUFFER[0]; //second part of cluster number

       if((U8)cluster & 0x01) *p=hi;
       else
        {
         *p&=0xF0;
         *p|=hi;
        }

       FATStatus=1; // we have made an entry, so write before next FAT sector read
      }
     else
      {
       p = &FAT_IO_BUFFER[secoffset];

       if((U8)cluster & 0x01)
        {
         *p&=0x0F;
         *p++|=lo;
         *p=hi;
        } 
       else
        {
         *p++=lo;
         *p&=0xF0;
         *p|=hi;
        } 

       FATStatus=1; // we have made an entry, so write before next FAT sector read
      } 

    }//if(FATtype==FAT12)
#endif

#ifdef USE_FAT16
   if(FATtype==FAT16)
    {
     //two bytes per FAT entry
//     sector=FATFirstSector + ((U32)cluster * 2) / BYTE_PER_SEC;
//     sector=FATFirstSector + ((unsigned int)cluster) / (BYTE_PER_SEC/2);
     UpdateFATBuffer(FATFirstSector + (U8)( (U16)cluster >> 8 ));

     // Buffer index is max 511. So in any case we loose upper bits. typecast cluster to U16
     p = &FAT_IO_BUFFER[((U16)cluster << 1) & (BYTE_PER_SEC-1)];

     *p++ = (U8)(number);
     *p   = (U8)(number >> 8);

     FATStatus=1; // we have made an entry, so write before next FAT sector read
    }// if(FATtype==FAT16)
#endif //#ifdef USE_FAT16

#ifdef USE_FAT32
   if(FATtype==FAT32)
    {
     //four bytes per FAT entry
//     sector=FATFirstSector + (cluster * 4) / BYTE_PER_SEC;
//     sector=FATFirstSector + cluster / (BYTE_PER_SEC/4);
     UpdateFATBuffer(FATFirstSector + ( cluster >> 7 ));

     // Buffer index is max 511. So in any case we loose upper bits. typecast cluster to U16
     p = &FAT_IO_BUFFER[((U16)cluster << 2) & (BYTE_PER_SEC-1)];

     number&=0x0FFFFFFF;
     
     *p++ = (U8)( number);
     *p++ = (U8)(number >> 8);
     *p++ = (U8)(number >> 16);
     *p   = (U8)(number >> 24);

     FATStatus=1; // we have made an entry, so write before next FAT sector read

    }// if(FATtype==FAT32) 
#endif //#ifdef USE_FAT32
  } // if(cluster<maxcluster) //we need to check this ;-)

 return 0;
}
#endif //DOS_WRITE

//###########################################################
/*!\brief Get drive informations
 * \return 		F_OK if successfull, F_ERROR if not
 *
 * This function is most important for the FAT filesystem.
 * Following values will be read out:
 * Type of the FAT filesystem.
 * Number of sectors for the partition (if there is one).
 * Number of clusters of the drive.
 * Where is the rootdirectory. And many more.
 * When using MMC/SD cards, call MMC_IO_Init() BEFORE GetDriveInformation() !
 */
U8 GetDriveInformation(void)
//###########################################################
{
 U8 by;
 U32 DataSec,TotSec;
 U32 bootSecOffset;
 U32 FATSz; // FATSize
 U8 loop;

#ifdef USE_FAT32
  U32 CountofClusters;
#else
//  U16 CountofClusters;  // for standard division
  U32 CountofClusters;    // for shifting division
#endif

 U16  RootEntrys;

 struct MBR *mbr;
 struct BootSec *boot;
  
 by=IdentifyMedia(); //LaufwerksInformationen holen
 if(by==0)
  {
   FATtype=0; //Unknown FAT type
   bootSecOffset=0; //erstmal

   by=ReadSector(0,iob); //Lese den MBR. Erster Sektor auf der Platte
                       //enthält max. 4 Partitionstabellen mit jeweils 16Bytes
                       //Die erste fängt bei 0x01BE an, und nur die nehme ich !

   //Erstmal checken ob wir nicht schon einen Bootsektor gelesen haben.
   boot=(struct BootSec *)iob;

   loop=0;
   do
    {
     // Jetzt checke ich doch den FAT-String im Bootsektor um den Typ der FAT
     // zu bestimmen. Einen besseren Weg sehe ich im Moment nicht.
     if(   boot->eb.rm.BS_FilSysType[0]=='F'
//        && boot->eb.rm.BS_FilSysType[1]=='A'
//        && boot->eb.rm.BS_FilSysType[2]=='T'
        && boot->eb.rm.BS_FilSysType[3]=='1'  )
      {
       //Wenn ich hier ankomme habe ich entweder FAT12 oder FAT16
#ifdef USE_FAT12
       if(boot->eb.rm.BS_FilSysType[4]=='2') FATtype=FAT12;
#endif
#ifdef USE_FAT16
       if(boot->eb.rm.BS_FilSysType[4]=='6') FATtype=FAT16;
#endif
      }
     else
      {
#ifdef USE_FAT32
       if(   boot->eb.rm32.BS_FilSysType[0]=='F'
//          && boot->eb.rm32.BS_FilSysType[1]=='A'
//          && boot->eb.rm32.BS_FilSysType[2]=='T'
          && boot->eb.rm32.BS_FilSysType[3]=='3'
          && boot->eb.rm32.BS_FilSysType[4]=='2'
         )
        {
         FATtype=FAT32;
        }
       else //war kein Bootsektor, also feststellen wo der liegt
#endif
        {
         mbr=(struct MBR *)iob; //Pointer auf die Partitionstabelle
         bootSecOffset=mbr->part1.bootoffset; //Nur den brauche ich

         by=ReadSector(bootSecOffset,iob);      //read bootsector
         boot=(struct BootSec *)iob;
        } 
      }

     loop++;
    }while(loop<2 && FATtype==0); //Bis zu zwei Versuche den Bootsektor zu lesen

   if(FATtype==0)
    {
#ifdef FAT_DEBUG_SHOW_FAT_INFO
     puts("FAT unknown\n");
#endif
     return F_ERROR; // FAT-Typ nicht erkannt
    }

   secPerCluster=boot->BPB_SecPerClus; //Sectors per Cluster
   RootEntrys=boot->BPB_RootEntCnt;    //32 Byte Root Directory Entrys
   RootDirSectors = (unsigned char)( ((RootEntrys * 32) + (BYTE_PER_SEC - 1)) / BYTE_PER_SEC);

//Number of sectors for FAT
   if(boot->BPB_FATSz16 != 0) FATSz = boot->BPB_FATSz16;
   else FATSz = boot->eb.rm32.BPB_FATSz32; //Für FAT32

   if(boot->BPB_TotSec16 != 0) TotSec = boot->BPB_TotSec16;
   else TotSec = boot->BPB_TotSec32;

   FATFirstSector= bootSecOffset + boot->BPB_RsvdSecCnt;
   FirstRootSector = FATFirstSector + (boot->BPB_NumFATs * FATSz);

//Number of data sectors
   DataSec = TotSec - (boot->BPB_RsvdSecCnt + (boot->BPB_NumFATs * FATSz) + RootDirSectors);

   FirstDataSector = FirstRootSector + RootDirSectors;

//Number of valid clusters
   //CountofClusters = DataSec / secPerCluster;
   U8 temp;

   // secPerCluster is always power of two 
   temp = secPerCluster>>1; // don't divide by 1 ;)
   CountofClusters = DataSec;
   
   while(temp)
    {
     CountofClusters >>= 1;
     temp >>= 1;
    }

   maxcluster = CountofClusters + 2;

//Note also that the CountofClusters value is exactly that: the count of data clusters
//starting at cluster 2. The maximum valid cluster number for the volume is
//CountofClusters + 1, and the "count of clusters including the two reserved clusters"
// is CountofClusters + 2.

   FirstDirCluster=0; // for FAT12 and FAT16

#ifdef USE_FAT12
   if(FATtype==FAT12)
    {
     endofclusterchain=EOC12;
    } 
#endif
#ifdef USE_FAT16
   if(FATtype==FAT16)
    {
     endofclusterchain=EOC16;
    } 
#endif
#ifdef USE_FAT32
   if(FATtype==FAT32)
    {
     endofclusterchain=EOC32;
     FAT32RootCluster=boot->eb.rm32.BPB_RootClus;
     FirstDirCluster=FAT32RootCluster;
     FirstRootSector=GetFirstSectorOfCluster(FAT32RootCluster);
    }
#endif

  }
 else
  {
   return F_ERROR; // CF gives no answer
  } 

 FileFirstCluster=0;
 FileSize=0;
 FileFlag=0;

 BytesPerCluster=BYTE_PER_SEC * secPerCluster; //bytes per cluster

 FATCurrentSector=FATFirstSector;

// for debugging only
#ifdef FAT_DEBUG_SHOW_FAT_INFO
 if(FATtype==FAT12) puts("FAT12\n");
 if(FATtype==FAT16) puts("FAT16\n");
 if(FATtype==FAT32) puts("FAT32\n");

 printf("bootSecOffset %lu\n",bootSecOffset);
 printf("Reserved Sectors %u\n",boot->BPB_RsvdSecCnt);
 printf("FAT Sectors %lu\n",FATSz);
 printf("Num. of FAT's %u\n",(U16)boot->BPB_NumFATs);
 printf("secPerCluster %u\n",(U16)secPerCluster);

#ifdef USE_64k_CLUSTERS
 printf("BytesPerCluster %lu\n",BytesPerCluster);
#else
 printf("BytesPerCluster %u\n",BytesPerCluster);
#endif

 printf("FATFirstSector %lu\n",FATFirstSector);
 printf("FirstRootSector %lu\n",FirstRootSector);
 printf("RootDirSectors %u\n",(U16)RootDirSectors);
 printf("FirstDataSector %lu\n",FirstDataSector);
 printf("maxsect %lu\n",maxsect);
#ifdef USE_FAT32
 printf("FirstDirCluster %lu\n",FirstDirCluster);
 printf("maxcluster %lu\n",maxcluster);
#else
 printf("FirstDirCluster %u\n",FirstDirCluster);
 printf("maxcluster %u\n",maxcluster);
#endif

#endif //#ifdef FAT_DEBUG_SHOW_FAT_INFO

 #ifdef DOS_WRITE
  FATStatus=0; // nothing to write til here
 #endif

 ReadFATSector(FATCurrentSector,FAT_IO_BUFFER); //read first FAT sector

 return F_OK;
}

#ifndef USE_FATBUFFER
#ifdef DOS_WRITE
//#########################################################################
U8 WriteFATSector(U32 sector, U8 *buf)
//#########################################################################
{
 iob_status=IOB_FAT; // we have to track this
 return WriteSector(sector,buf); //write sector
}
#endif

//#########################################################################
U8 ReadFATSector(U32 sector, U8 *buf)
//#########################################################################
{
 iob_status=IOB_FAT; // we have to track this
 return ReadSector(sector,buf); //read sector
}

#ifdef DOS_WRITE // Better keep this as a function !
//#########################################################################
U8 WriteDirSector(U32 sector, U8 *buf)
//#########################################################################
{
 iob_status=IOB_DIR; // we have to track this
 return WriteSector(sector,buf); //write sector
}
#endif

//#########################################################################
U8 ReadDirSector(U32 sector, U8 *buf)
//#########################################################################
{
 iob_status=IOB_DIR; // we have to track this
 return ReadSector(sector,buf); //read sector
}

#ifdef DOS_WRITE // This has to be a function for Fwrite(), Fclose() !
//#########################################################################
U8 WriteFileSector(U32 sector, U8 *buf)
//#########################################################################
{
 iob_status=IOB_DATA; // we have to track this
 return WriteSector(sector,buf); //write sector
}
#endif

//#########################################################################
U8 ReadFileSector(U32 sector, U8 *buf)
//#########################################################################
{
 iob_status=IOB_DATA; // we have to track this
 return ReadSector(sector,buf); //read sector
}

#endif //#ifndef USE_FATBUFFER
//@}
