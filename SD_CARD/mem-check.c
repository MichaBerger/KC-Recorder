//http://www.roboternetz.de/wissen/index.php/Speicherverbrauch_bestimmen_mit_avr-gcc
#include <avr/io.h>  // RAMEND
#include "mem-check.h"

// This does not work if you use malloc(), or fdevopen() for printf() of avr-libc !
// Use FDEV_SETUP_STREAM() for printf() instead.

// Mask to init SRAM and check against
#define MASK 0xaa

unsigned short get_mem_unused (void)
{
   unsigned short unused = 0;
   unsigned char *p = &__heap_start;

   do
   {
      if (*p++ != MASK)
         break;

      unused++;
   } while (p <= (unsigned char*) RAMEND);

      return unused;
}

void init_mem(void) __attribute__((naked)) __attribute__((section (".init3")));

/* !!! never call this function !!! */
void init_mem (void)
{
   __asm volatile (
      "ldi r30, lo8 (__heap_start)"  "\n\t"
      "ldi r31, hi8 (__heap_start)"  "\n\t"
      "ldi r24, %0"                  "\n\t"
      "ldi r25, hi8 (%1)"            "\n"
      "0:"                           "\n\t"
      "st  Z+,  r24"                 "\n\t"
      "cpi r30, lo8 (%1)"            "\n\t"
      "cpc r31, r25"                 "\n\t"
      "brlo 0b"
         :
         : "i" (MASK), "i" (RAMEND+1)
   );
}
