/* 
 * The various functions required by 6502core.c to function properly on 
 * the Palm platform.
 * The event-driven user interface code and the startup code is all 
 * located in palmui.c, look there for the rest of the Palm code. 
 * sdyoung@well.com */

#include "palmcore.h"
#include "6502core.h"
#ifdef PALM
#include <Pilot.h>
#include "callback.h"
#include "palmcoreRsc.h"
#else
#include <string.h>
#endif

void xdebug(char *str, word_t val) {
#ifdef PALM   
     CharPtr buf[256];
     StrIToA(buf, val);
     FrmCustomAlert(ErrorAlert, (CharPtr *) str, buf, NULL);
#else
     printf("%s%X\n", str, val);
#endif
}


#ifdef PALM
VoidPtr xmemset(VoidPtr b, int c, word_t len) {
     word_t count;
     VoidPtr a;
   
     for(count=0, a=b;count<len;count++, a++) 
        *a=c;
     
     return(b);
}
#else
void *xmemset(void *b, int c, size_t len) {
     return(memset(b, c, len));
}
#endif /* PALM */

/* that's all for the functions required by the UNIX version.  The rest 
 * is specifically for the Palm version. */
#ifdef PALM

char memory[MEMSIZE];

byte_t readmembyte(word_t address) {
     if(address>MEMSIZE) {
	  xdebug("Attempted to read >MEMSIZE: ", address);
	  return(0);
     }
     return(memory[address]);
}

void writemembyte(word_t address, byte_t data) {
     if(address>MEMSIZE) {
	  xdebug("Attempted to write >MEMSIZE: ", address);
	  return;
     }
     memory[address]=data;
}
#endif /* PALM */
