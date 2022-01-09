/*! \file find_x.c \brief Directory-Find-Functions */
//###########################################################
///	\ingroup singlefat
///	\defgroup find DIR-Find-Functions (find_x.c)
///	\code #include "find_x.h" \endcode
///	\code #include "dir.h" \endcode
///	\code #include "dos.h" \endcode
///	\par Uebersicht
//###########################################################
// Find first file in a directory and give back filename as
// a NULL terminated string. Also fileattr and filesize.
//
// ffblk.ff_name[]   	8.3 DOS name with '.' in it and \0 at the end
// ffblk.ff_longname[]  Long filename with \0 at the end
// ffblk.ff_attr     	ATTR_FILE or ATTR_DIRECTORY
// ffblk.ff_fsize    	Filesize, 0 if directory
//
// Use this data to find next file, next file, ... in a directory.
//
// Necessary to make a "dir" or "ls" command. Or opening files
// with fopen() without knowing the filename of the first,next,next... file.
//
// This doesn't work without initialized global variable FirstDirCluster
// which points to the current directory.
//
// 12.11.2006 Most code simply merged into dir.c
//
// 26.12.2005 Found a bug in constructing the long name. If it had exactly
//            13 Bytes, parts of the last findnext() are in ffblk.ff_longname. 
//
// Benutzung auf eigene Gefahr !
//
// Use at your own risk !
//
//#########################################################################
// Last change: 10.08.2007
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

#ifdef USE_FINDFILE

struct FindFile ffblk; 	//make this global to avoid parameter passing


//###########################################################
/*!\brief Finds the first file or dir entry in current directory
 * \return 0 if no file is found,1 if a file is found
 *
 * Check ffblk.ff_attr if you have a file or a dir entry
 */
U8 Findfirst(void)
//###########################################################
{
 ffblk.lastposition=0;                	//no position
 return Findnext();
}

//###########################################################
/*!\brief Finds the next file or dir entry in current directory
 * \return 0 if no file is found,1 if a file is found
 *
 * Always starts to search from the beginning of a directory !
 * Check ffblk.ff_attr if you have a file or a dir entry.
 * NEVER call this before calling Findfirst() !
 * Your program crashes and your hardware will be destroyed.
 */
U8 Findnext(void)
//###########################################################
{
 U8 i;

#ifdef USE_FINDLONG
 for(i=0; i<_MAX_NAME; i++) ffblk.ff_longname[i]=0;	// clean the long filename.
#endif

 //delete last data
 for(i=0; i<13; i++) ffblk.ff_name[i]=0; //clean last filename

 ffblk.ff_attr=ATTR_NO_ATTR; 	      	//no file attr
 ffblk.ff_fsize=0; 	      		//no filesize
 ffblk.newposition=0; //no position for next search

 if(ScanDirectory(NULL)==FULL_MATCH) return 1;

 return 0;
}

#endif // #ifdef USE_FINDFILE
//@}


