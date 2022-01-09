#ifndef _MEM_CHECK_H_
#define _MEM_CHECK_H_

// From linker script
// Holger Klabunde: Wieso unsigned char ?
extern unsigned char __heap_start;

extern unsigned short get_mem_unused (void);

#endif  /* _MEM_CHECK_H_ */
