/*
 * The 6502 emulation-fu for Astraea
 * FIXMEs: See top of setupmachine()
 *         Stack operations should check for under/overflow?
 *         A debugging facility should be provided
 *         Make sure P is really the way it should be. (unused flags etc)
 *         **Flag-fu could be written in a much better way, however it
 *           it currently patched up from when I did it incorrectly**
 *         SBC (0xE1) and ISB (0xE3) are currently unimplemented
 *         ARR doesn't do exactly as it should, RTFM
 *
 * Teknovore/1997,1998
 * Julian Squires 1999
 *
 * $Id: 6502core.c,v 1.2 1999/08/05 00:46:25 tek Exp $
 *
 */

#ifndef PALM
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "6502core.h"

extern byte_t readmembyte(word_t address);
extern void writemembyte(word_t address, byte_t data);
extern void *xmemset(void *, char value, size_t size);

#ifdef PALM
#include "palmcore.h"
#endif /* PALM */
/*
 * FIXME: This currently needs to detect which endian the machine is.
 * Right now it just pretends it checks.
 * It also only handles little and big endian.
 */

char setupmachine(registers_t *registers)
{
     int endian = 0;

     registers->A = 0;
     registers->X = 0;
     registers->Y = 0;
     registers->S = 0;
     registers->P = 0;

     registers->PC = 0x8000;
     /* We should be checking the endian-ness here */
#ifdef PALM
     endian = 2;
#else 
     endian = 1;
#endif /* PALM */
     if(endian == 1) {		/* Little endian */
	  registers->PCL = (byte_t *)&registers->PC;
	  registers->PCH = (byte_t *)(&registers->PC+1);
     } else if(endian == 2) {	/* Big endian */
	  registers->PCH = (byte_t *)&registers->PC;
	  registers->PCL = (byte_t *)(&registers->PC+1);
     } else {			/* Bomb! */
	  return(1);
     }
     return(0);
}

/*
 * What we really pray for here is that the giant switch gets compiled
 * into a jump table.
 */

void cpustep(registers_t *registers)
{
     register byte_t foo;	/* Never assume foo contains anything! */
     register word_t bar;	/* Ditto */
     register word_t baz;	/* Ditto */

     switch(readmembyte(registers->PC)) {
     case 0x00:			/* BRK -> Break */
	  registers->PC++;
	  writemembyte(0x100+(word_t)registers->S, *registers->PCH);
	  writemembyte(0x100+(word_t)registers->S+1, *registers->PCL);
	  registers->S+=(byte_t)2;
	  registers->P |= 0x10;	/* Set break flag */
	  writemembyte(0x100+(word_t)registers->S, registers->P);
	  registers->S++;
	  registers->P &= ~0x04; /* Clear interrupt */
	  *registers->PCL = readmembyte(0xFFFE);
	  *registers->PCH = readmembyte(0xFFFF);
	  /* Count 7 ticks here */
	  break;
     case 0x01:			/* ORA -> Logical Inclusive OR */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;
	  registers->A |= readmembyte((word_t)(readmembyte(registers->X+foo)|
				      (readmembyte((word_t)
						   (registers->X+foo+
						    (byte_t)1))<<8)));
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02,
		 (registers->A == (byte_t)0)<<(unsigned)1); /* Zero */
	  /* Count 6 ticks here */
	  break;
     case 0x02:			/* Jam! */
	  break;
     case 0x03:			/* SLO -> Shift left, OR A */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;
	  registers->A |= readmembyte((word_t)(readmembyte(registers->X+foo)|
				      readmembyte(registers->X+foo+(byte_t)1)
				      <<8))<<1;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 8 ticks here */
	  break;
     case 0x04:			/* NOP -> No operation */
	  registers->PC+=2;
	  /* Count 3 ticks here */
	  break;
     case 0x05:			/* ORA -> Logical Inclusive OR */
	  registers->A |= readmembyte(readmembyte(registers->PC+1));
	  registers->PC+=2;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 3 ticks here */
	  break;
     case 0x06:			/* ASL -> Arith shift left */
	  foo = readmembyte(readmembyte(registers->PC+1));
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo<<=1;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(readmembyte(registers->PC+1), foo);
	  registers->PC+=2;
	  /* Count 5 ticks here */
	  break;
     case 0x07:			/* SRA -> ASL + ORA?? */
	  foo = readmembyte(readmembyte(registers->PC+1));
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  foo |= registers->A;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(readmembyte(registers->PC+1), foo);
	  registers->PC+=2;
	  /* Count 5 ticks here */
	  break;
     case 0x08:			/* PHP -> Push P */
	  registers->PC++;
	  writemembyte(0x100+registers->S, registers->P);
	  registers->S++;
	  /* Count 3 ticks here */
	  break;
     case 0x09:			/* ORA */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;
	  registers->A |= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 2 ticks here */
	  break;
     case 0x0A:			/* ASL */
	  registers->PC++;
	  registers->A <<= 1;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 2 ticks here */
	  break;
     case 0x0B:			/* ANC */
	  registers->A&=(registers->A&readmembyte(registers->PC+1));
	  registers->PC+=2;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 2 ticks here */
	  break;
     case 0x0C:			/* NOP */
	  registers->PC+=3;
	  /* Count 4 ticks here */
	  break;
     case 0x0D:			/* ORA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;
	  foo = readmembyte(bar);
	  registers->A |= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 4 ticks here */
	  break;
     case 0x0E:			/* ASL */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;
	  foo = readmembyte(bar);
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;
     case 0x0F:			/* SRA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;
	  foo = readmembyte(bar);
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  foo |= registers->A;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;
     case 0x10:			/* BPL */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;
	  if(!registers->P&0x80) {
	       bar = *registers->PCH;
	       registers->PC += (signed char)foo;
	       if(*registers->PCH == bar) { /* Same page */
		    /* Count 3 ticks here */
	       } else {
		    /* Count 4 ticks here */
	       }
	  } else
	       /* Count 2 ticks here */
	  break;
     case 0x11:			/* ORA */
	  foo = readmembyte(registers->PC+1);
	  bar = readmembyte(foo)|(readmembyte(foo+1)<<8);
	  foo = readmembyte(registers->Y + bar);
	  registers->PC+=2;
	  if((bar&0xff)+registers->Y > 0xFF) {
	       /* Count 6 ticks here */
	  } else {
	       /* Count 5 ticks here */
	  }
	  registers->A |= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  break;
     case 0x12:			/* Jam! */
	  break;
     case 0x13:			/* SRA */
	  foo = readmembyte(registers->PC+1);
	  bar = readmembyte(foo)|(readmembyte(foo+1)<<8);
	  foo = readmembyte(registers->Y + bar);
	  registers->PC+=2;
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  foo |= registers->A;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar+registers->Y, foo);
	  /* Count 8 ticks here */
	  break;
     case 0x14:			/* NOP */
	  registers->PC+=2;
	  /* Count 4 ticks here */
	  break;
     case 0x15:			/* ORA */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;
	  registers->A |= readmembyte(foo+registers->X);
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 4 ticks here */
	  break;
     case 0x16:			/* ASL */
	  bar = readmembyte(registers->PC+1)+registers->X;
	  registers->PC+=2;
	  foo = readmembyte(bar);
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;
     case 0x17:			/* SRA */
	  bar = readmembyte((registers->PC+1)+registers->X);
	  foo = readmembyte(bar);
	  registers->PC+=2;
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  foo |= registers->A;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;
     case 0x18:			/* CLC -> Clear carry */
	  registers->PC++;
	  registers->P &= ~0x01;
	  /* Count 2 ticks here */
	  break;
     case 0x19:			/* ORA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;
	  foo = readmembyte(bar+registers->Y); 
	  if(registers->Y+(bar&0xFF)>0xFF) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }
	  foo |= registers->A;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar+registers->Y, foo);
	  break;
     case 0x1A:			/* NOP */
	  registers->PC++;
	  /* Count 2 ticks here */
	  break;
     case 0x1B:			/* SRA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;
	  foo = readmembyte(bar+registers->Y);
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  foo |= registers->A;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar+registers->Y, foo);
	  /* Count 7 ticks here */
	  break;
     case 0x1C:			/* NOP */
	  registers->PC+=3;
	  /* Count 4 ticks here */
	  break;
     case 0x1D:			/* ORA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;
	  foo = readmembyte(bar+registers->X);
	  if(registers->X+(bar&0xFF)>0xFF) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }
	  foo |= registers->A;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar+registers->X, foo);
	  break;
     case 0x1E:			/* ASL */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;
	  foo = readmembyte(bar+registers->X);
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar+registers->X, foo);
	  /* Count 7 ticks here */
	  break;
     case 0x1F:			/* SRA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;
	  foo = readmembyte(bar+registers->X);
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  foo |= registers->A;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar+registers->X, foo);
	  /* Count 7 ticks here */
	  break;
     case 0x20:			/* JSR */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;
	  writemembyte(0x100+registers->S, *registers->PCH);
	  writemembyte(0x100+registers->S+1, *registers->PCL);
	  registers->S+=2;
	  registers->PC = bar;
	  /* Count 6 ticks here */
	  break;
     case 0x21:			/* AND */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;
	  registers->A &= readmembyte(readmembyte(registers->X+foo)|
				      (readmembyte(registers->X+foo+1)<<8));
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 6 ticks here */
	  break;
     case 0x22:			/* Jam! */
	  break;
     case 0x23:			/* RLA -> ROL with AND */
	  foo = readmembyte(registers->PC+1);
	  foo = readmembyte(readmembyte(registers->X+foo)|
			    (readmembyte(registers->X+foo+1)<<8));
	  registers->PC+=2;
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  if(registers->P&0x01)
	       foo |= 0x01;
	  registers->A &= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  writemembyte(bar+registers->X, foo);
	  /* Count 6 ticks here */
	  break;
     case 0x24:			/* BIT */
	  foo = readmembyte(readmembyte(registers->PC+1));
	  registers->PC+=2;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x40, foo&0x40); /* Overflow (bit 6) */
	  setbit(registers->P, 0x02, (!(foo&registers->A))<<1); /* Zero */
	  /* Count 3 ticks here */
	  break;
     case 0x25:			/* AND */
	  foo = readmembyte(readmembyte(registers->PC+1));
	  registers->PC+=2;
	  registers->A &= readmembyte(foo+registers->X);
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 3 ticks here */
	  break;
     case 0x26:			/* ROL */
	  bar = readmembyte(registers->PC+1);
	  foo = readmembyte(bar);
	  registers->PC+=2;
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  if(registers->P&0x01)
	       foo |= 0x01;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 5 ticks here */
	  break;
     case 0x27:			/* RLA */
	  bar = readmembyte(registers->PC+1);
	  foo = readmembyte(bar);
	  registers->PC+=2;
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  if(registers->P&0x01)
	       foo |= 0x01;
	  registers->A &= foo;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 5 ticks here */
	  break;
     case 0x28:			/* PLP */
	  registers->PC++;
	  registers->P = readmembyte(0x100+registers->S);
	  registers->S--;
	  /* Count 4 ticks here */
	  break;
     case 0x29:			/* AND */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;
	  registers->A &= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 2 ticks here */
	  break;
     case 0x2A:			/* ROL */
	  registers->PC++;
	  setbit(registers->P, 0x01, (registers->A&0x80)>>7); /* Carry */
	  registers->A <<= 1;
	  if(registers->P&0x01)
	       registers->A |= 0x01;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 2 ticks here */
	  break;
     case 0x2B:			/* ANC -> AND with carry? */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;
	  registers->A &= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 2 ticks here */
	  break;
     case 0x2C:			/* BIT */
	  foo = readmembyte(readmembyte(registers->PC+1)|
			    (readmembyte(registers->PC+2)<<8));
	  registers->PC+=3;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x40, foo&0x40); /* Overflow (bit 6) */
	  setbit(registers->P, 0x02, (!(foo&registers->A))<<1); /* Zero */
	  /* Count 4 ticks here */
	  break;
     case 0x2D:			/* AND */
	  foo = readmembyte(readmembyte(registers->PC+1)|
			    (readmembyte(registers->PC+2)<<8));
	  registers->PC+=3;
	  registers->A &= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 4 ticks here */
	  break;
     case 0x2E:			/* ROL */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  if(registers->P&0x01)
	       foo |= 0x01;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;
     case 0x2F:			/* RLA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  if(registers->P&0x01)
	       foo |= 0x01;
	  registers->A &= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;
     case 0x30:			/* BMI -> Branch MInus*/
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;
	  if(registers->P&0x80) {
	       bar = registers->PC&0xFF00;
	       registers->PC = registers->PC+foo;
	       if((registers->PC&0xFF00)== bar) {
		    /* Count 3 ticks here */
	       } else {
		    /* Count 4 ticks here */
	       }
	  } else {
	       /* Count 2 ticks here */
	  }
	  break;
     case 0x31:			/* AND */
	  foo = readmembyte(registers->PC+1);
	  bar = readmembyte(foo)|(readmembyte(foo+1)<<8);
	  foo = readmembyte(bar+registers->Y);
	  registers->PC+=2;
	  if((bar&0x00FF)+registers->Y > 0xFF) {
	       /* Count 6 ticks here */
	  } else {
	       /* Count 5 ticks here */
	  }
	  registers->A &= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  break;
     case 0x32:			/* Jam! */
	  break;
     case 0x33:			/* RLA */
	  foo = readmembyte(registers->PC+1);
	  bar = readmembyte(foo)|(readmembyte(foo+1)<<8);
	  foo = readmembyte(bar+registers->Y);
	  registers->PC+=2;
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  if(registers->P&0x01)
	       foo |= 0x01;
	  registers->A &= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  writemembyte(bar+registers->Y, foo);
	  /* Count 8 ticks here */
	  break;
     case 0x34:			/* NOP */
	  registers->PC += 2;
	  /* Count 4 ticks here */
	  break;
     case 0x35:			/* AND */
	  foo = readmembyte(readmembyte(registers->PC+1)+registers->X);
	  registers->PC+=2;
	  registers->A &= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 4 ticks here */
	  break;
     case 0x36:			/* ROL */
	  foo = readmembyte(bar = (readmembyte(registers->PC+1)+registers->X));
	  registers->PC+=2;
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  if(registers->P&0x01)
	       foo |= 0x01;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;
     case 0x37:			/* RLA */
	  foo = readmembyte(bar = (readmembyte(registers->PC+1)+registers->X));
	  registers->PC+=2;
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  if(registers->P&0x01)
	       foo |= 0x01;
	  registers->A &= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;
     case 0x38:			/* SEC - Set carry */
	  registers->PC++;
	  registers->P |= 0x01;	/* Carry */
	  /* Count 2 ticks here */
	  break;
     case 0x39:			/* AND */
	  bar = readmembyte(registers->PC+1)|(readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->Y);
	  registers->PC+=3;
	  if(((bar&0xFF)+registers->Y) > 0xFF) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }
	  registers->A &= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  break;
     case 0x3A:			/* NOP */
	  registers->PC++;
	  /* Count 2 ticks here */
	  break;
     case 0x3B:			/* RLA */
	  bar = readmembyte(registers->PC+1)|(readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->Y);
	  registers->PC += 3;
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  if(registers->P&0x01)
	       foo |= 0x01;
	  registers->A &= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  writemembyte(bar+registers->Y, foo);
	  /* Count 7 ticks here */
	  break;
     case 0x3C:			/* NOP */
	  registers->PC += 3;
	  /* Count 4 ticks here */
	  break;
     case 0x3D:			/* AND */
	  bar = readmembyte(registers->PC+1)|(readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->X);
	  registers->PC+=3;
	  if(((bar&0xFF)+registers->X) > 0xFF) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }
	  registers->A &= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  break;
     case 0x3E:			/* ROL */
	  bar = readmembyte(registers->PC+1)|(readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->X);
	  registers->PC+=3;
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  if(registers->P&0x01)
	       foo |= 0x01;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar+registers->X, foo);
	  /* Count 7 ticks here */
	  break;
     case 0x3F:			/* RLA */
	  bar = readmembyte(registers->PC+1)|(readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->X);
	  registers->PC+=3;
	  setbit(registers->P, 0x01, (foo&0x80)>>7); /* Carry */
	  foo <<= 1;
	  if(registers->P&0x01)
	       foo |= 0x01;
	  registers->A &= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  writemembyte(bar+registers->X, foo);
	  /* Count 7 ticks here */	  
	  break;
     case 0x40:			/* RTI */
	  registers->PC++;
	  registers->P = readmembyte(0x100+registers->S--);
	  registers->PC = readmembyte(0x100+registers->S--)|
	       (readmembyte(0x100+registers->S--)<<8);
	  /* Count 6 ticks here */
	  break;
     case 0x41:			/* EOR */
	  foo = readmembyte(registers->PC+1);
	  foo = readmembyte(readmembyte(foo+registers->X)|
			    (readmembyte(foo+registers->X)<<8));
	  registers->PC+=2;
	  registers->A ^= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 6 ticks here */
     case 0x42:			/* Jam! */
	  break;
     case 0x43:			/* SRE */
	  foo = readmembyte(registers->PC+1);
	  bar = readmembyte(foo+registers->X)|
	       (readmembyte(foo+registers->X)<<8);
	  foo = readmembyte(bar);
	  if(foo&0x01)		/* Carry */
	       registers->P |= 0x01;
	  foo >>= 1;
	  foo ^= registers->A;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 8 ticks here */
	  break;
     case 0x44:			/* NOP */
	  registers->PC+=2;
	  /* Count 3 ticks here */
	  break;
     case 0x45:			/* EOR */
	  foo = readmembyte(readmembyte(registers->PC+1));
	  registers->PC+=2;
	  registers->A ^= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 3 ticks here */
	  break;
     case 0x46:			/* LSR */
	  bar = readmembyte(registers->PC+1);
	  foo = readmembyte(bar);
	  registers->PC+=2;
	  if(foo&0x01)		/* Carry */
	       registers->P |= 0x01;
	  foo >>= 1;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 5 ticks here */
	  break;
     case 0x47:			/* SRE */
	  bar = readmembyte(registers->PC+1);
	  foo = readmembyte(bar);
	  registers->PC+=2;
	  if(foo&0x01)		/* Carry */
	       registers->P |= 0x01;
	  foo >>= 1;
	  registers->A ^= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 5 ticks here */	  
	  break;
     case 0x48:			/* PHA */
	  registers->PC++;
	  writemembyte(0x100+registers->S, registers->A);
	  registers->S++;
	  /* Count 3 ticks here */
	  break;
     case 0x49:			/* EOR */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;
	  registers->A ^= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 2 ticks here */
	  break;
     case 0x4A:			/* LSR */
	  registers->PC++;
	  if(registers->A&0x01)		/* Carry */
	       registers->P |= 0x01;
	  registers->A >>= 1;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 2 ticks here */
	  break;
     case 0x4B:			/* ASR */
	  bar = readmembyte(registers->PC+1);
	  foo = registers->A&bar;
	  registers->PC+=2;
	  if(foo&0x01)		/* Carry */
	       registers->P |= 0x01;
	  foo >>= 1;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  registers->A = foo;
	  /* Count 2 ticks here */
	  break;
     case 0x4C:			/* JMP */
	  bar = readmembyte(registers->PC+1)|(readmembyte(registers->PC+2)<<8);
	  registers->PC = bar;
	  /* Count 3 ticks here */
	  break;
     case 0x4D:			/* EOR */
	  bar = readmembyte(registers->PC+1)|(readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;
	  registers->A ^= readmembyte(bar);
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 4 ticks here */
	  break;
     case 0x4E:			/* LSR */
	  bar = readmembyte(registers->PC+1)|(readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;
	  if(foo&0x01)		/* Carry */
	       registers->P |= 0x01;
	  foo >>= 1;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;
     case 0x4F:			/* SRE */
	  bar = readmembyte(registers->PC+1)|(readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;
	  if(foo&0x01)		/* Carry */
	       registers->P |= 0x01;
	  foo >>= 1;
	  registers->A ^= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;
     case 0x50:			/* BVC */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;
	  if(!registers->P & 0x40) {
	       if((registers->PC&0x00FF)+foo > 0x00FF) {
		    /* Count 4 ticks here */
	       } else {
		    /* Count 3 ticks here */
	       }
	       registers->PC += foo;
	  } else {
	       /* Count 2 ticks here */
	  }
	  break;
     case 0x51:			/* EOR */
	  foo = readmembyte(registers->PC+1);
	  bar = readmembyte(foo)|(readmembyte(foo)<<8);
	  foo = readmembyte(bar+registers->Y);
	  registers->PC+=2;
	  if((bar&0x00FF)+registers->Y > 0x00FF) {
	       /* Count 6 ticks here */
	  } else {
	       /* Count 5 ticks here */
	  }
	  registers->A ^= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  break;
     case 0x52:			/* Jam! */
	  break;
     case 0x53:			/* SRE */
	  foo = readmembyte(registers->PC+1);
	  bar = (readmembyte(foo)|(readmembyte(foo)<<8))+registers->Y;
	  foo = readmembyte(bar);
	  registers->PC+=2;
	  if(foo&0x01)		/* Carry */
	       registers->P |= 0x01;
	  foo >>= 1;
	  registers->A ^= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 8 ticks here */
	  break;
     case 0x54:			/* NOP */
	  registers->PC+=2;
	  /* Count 4 ticks here */
	  break;
     case 0x55:			/* EOR */
	  foo = readmembyte(registers->PC+1);
	  foo = readmembyte(foo+registers->X)|(readmembyte(foo+registers->X)<<8);
	  registers->PC+=2;
	  registers->A ^= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  /* Count 4 ticks here */
	  break;
     case 0x56:			/* LSR */
	  bar = readmembyte(registers->PC+1)+registers->X;
	  foo = readmembyte(bar);
	  registers->PC+=2;
	  if(foo&0x01)		/* Carry */
	       registers->P |= 0x01;
	  foo >>= 1;
	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;
     case 0x57:			/* SRE */
	  bar = readmembyte(registers->PC+1)+registers->X;
	  foo = readmembyte(bar);
	  registers->PC+=2;
	  if(foo&0x01)		/* Carry */
	       registers->P |= 0x01;
	  foo >>= 1;
	  registers->A ^= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;
     case 0x58:			/* CLI */
	  registers->PC++;
	  registers->P &= (~0x04); /* Clear int flag */
	  /* Count 2 ticks here */
	  break;
     case 0x59:			/* EOR */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->Y);
	  registers->PC+=3;
	  if(((bar+registers->Y)&0xFF00) != (bar&0xFF00)) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }
	  registers->A ^= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  break;
     case 0x5A:			/* NOP */
	  registers->PC++;
	  /* Count 2 ticks here */
	  break;
     case 0x5B:			/* SRE */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->Y);
	  registers->PC+=3;
	  if(foo&0x01)		/* Carry */
	       registers->P |= 0x01;
	  foo >>= 1;
	  registers->A ^= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  writemembyte(bar+registers->Y, foo);
	  /* Count 7 ticks here */
	  break;
     case 0x5C:			/* NOP */
	  registers->PC+=3;
	  /* Count 4 ticks here */
	  break;
     case 0x5D:			/* EOR */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->X);
	  registers->PC+=3;
	  if(((bar+registers->X)&0xFF00) != (bar&0xFF00)) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }
	  registers->A ^= foo;
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */
	  break;
/* FIXME: <0x5E need reformatting */
     case 0x5E:			/* LSR */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->X);
	  registers->PC+=3;

	  if(foo&0x01)		/* Carry */
	       registers->P |= 0x01;

	  foo >>= 1;

	  setbit(registers->P, 0x80, foo&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!foo)<<1); /* Zero */

	  writemembyte(bar+registers->X, foo);
	  /* Count 7 ticks here */
	  break;

     case 0x5F:			/* SRE */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->X);
	  registers->PC+=3;

	  if(foo&0x01)		/* Carry */
	       registers->P |= 0x01;

	  foo >>= 1;
	  registers->A ^= foo;

	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02, (!registers->A)<<1); /* Zero */

	  writemembyte(bar+registers->X, foo);	  
	  /* Count 7 ticks here */
	  break;

     case 0x60:			/* RTS */
	  bar = readmembyte(registers->S--)|
	       (readmembyte(registers->S--)<<8);
	  registers->PC = bar;
	  /* Count 6 ticks here */
	  break;

     case 0x61:			/* ADC */
	  foo = readmembyte(registers->PC+1)+registers->X;
	  bar = readmembyte(foo)|(readmembyte(foo)<<8);
	  foo = readmembyte(bar);
	  bar = foo+registers->A+(registers->P&0x01);
	  registers->PC+=2;
	  registers->P&=(~0x02)&(~bar);

	  if(registers->P&0x08) { /* Decimal mode */
	       if(((registers->A&0x0F)+(foo&0x0F)+(registers->P&0x01)) > 9)
		    bar+=6;

	       setbit(registers->P, 0x80, bar&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^bar)&0x80))<<6); /* Overflow */

	       if(bar > 0x99)
		    bar += 96;

	       setbit(registers->P, 0x01, (bar>0x99)?1:0); /* Carry */
	  } else {
	       setbit(registers->P, 0x80, bar&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^bar)&0x80))<<6); /* Overflow */
	       setbit(registers->P, 0x01, (bar>0xFF)?1:0); /* Carry */
	  }

	  registers->A = bar&0xFF;
	  /* Count 6 ticks here */
	  break;

     case 0x62:			/* Jam! */
	  break;

     case 0x63:			/* RRA */
	  bar = readmembyte(registers->PC+1);
	  foo = readmembyte(readmembyte(bar+registers->X));
	  registers->PC += 2;

	  baz = foo >> 1;

	  if(registers->P&0x01)
	       baz |= 0x80;

	  setbit(registers->P, 0x80, baz&0x80);
	  setbit(registers->P, 0x02, (baz)?0x00:0x02);
	  setbit(registers->P, 0x40, (baz^foo)&0x40);

	  if(registers->P&0x08) {
	       if((foo&0x0F) > 4)
		    baz = (baz&0xF0)|((baz+6)&0x0F);
	       if(foo > 0x4F)
		    baz += 96;

	       setbit(registers->P, 0x01, (foo > 0x4F)?1:0);
	  } else {
	       setbit(registers->P, 0x01, (foo&0x80)?1:0);
	  }

	  writemembyte(readmembyte(bar+registers->X), baz);

	  /* Count 8 ticks here */
	  break;

     case 0x64:			/* NOP */
	  registers->PC+=2;

	  /* Count 3 ticks here */
	  break;

     case 0x65:			/* ADC - stolen again */
	  bar = readmembyte(registers->PC+1);
	  foo = readmembyte(bar);
	  bar = foo+registers->A+(registers->P&0x01);
	  registers->PC += 2;

	  setbit(registers->P, 0x02, (!bar)?0x02:0x00);

	  if(registers->P&0x08) {
	       if(((registers->A&0x0F)+(foo&0x0F)+(registers->P&0x01)) > 9)
		    bar += 6;

	       setbit(registers->P, 0x80, bar&0x80);
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80)&&
				((registers->A^bar)&0x80))?0x40:0x00);

	       if(bar > 0x99)
		    bar+=96;

	       setbit(registers->P, 0x01, (bar > 0x99)?1:0);
	  } else {
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80)&&
				((registers->A^bar)&0x80))?0x40:0x00);
	       setbit(registers->P, 0x80, bar&0x80);
	       setbit(registers->P, 0x01, (bar>0xFF)?1:0);
	  }

	  registers->A = bar&0xFF;
	  /* Count 3 ticks here */
	  break;

     case 0x66:			/* ROR */
	  bar = readmembyte(registers->PC+1);
	  baz = readmembyte(bar);
	  registers->PC+=2;

	  if(registers->P&0x01)
	       baz |= 0x100;
	  
	  setbit(registers->P, 0x01, baz&0x01);
	  baz >>= 1;

	  setbit(registers->P, 0x80, baz&0x80);
	  setbit(registers->P, 0x02, (!baz)?0x02:0x00);

	  writemembyte(bar, baz&0xFF);
	  /* Count 5 ticks here */
	  break;

     case 0x67:			/* RRA */
	  bar = readmembyte(registers->PC+1);
	  foo = readmembyte(bar);
	  registers->PC+=2;

	  baz = foo >> 1;

	  if(registers->P&0x01)
	       baz |= 0x80;

	  setbit(registers->P, 0x80, baz&0x80);
	  setbit(registers->P, 0x02, (baz)?0x00:0x02);
	  setbit(registers->P, 0x40, (baz^foo)&0x40);

	  if(registers->P&0x08) {
	       if((foo&0x0F) > 4)
		    baz = (baz&0xF0)|((baz+6)&0x0F);
	       if(foo > 0x4F)
		    baz += 96;

	       setbit(registers->P, 0x01, (foo > 0x4F)?1:0);
	  } else {
	       setbit(registers->P, 0x01, (foo&0x80)?1:0);
	  }

	  writemembyte(bar, baz);

	  /* Count 5 ticks here */
	  break;

     case 0x68:			/* PLA */
	  registers->PC++;

	  foo = readmembyte(registers->S--);
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x00);

	  registers->A = foo;
	  /* Count 4 ticks here */
	  break;

     case 0x69:			/* ADC */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  bar = registers->A+foo;

	  if(registers->P&0x08) { /* Decimal mode */
	       if(((registers->A&0x0F)+(foo&0x0F)+(registers->P&0x01)) > 9)
		    bar+=6;

	       setbit(registers->P, 0x80, bar&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^bar)&0x80))<<6); /* Overflow */

	       if(bar > 0x99)
		    bar += 96;

	       setbit(registers->P, 0x01, (bar>0x99)?1:0); /* Carry */
	  } else {
	       setbit(registers->P, 0x80, bar&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^bar)&0x80))<<6); /* Overflow */
	       setbit(registers->P, 0x01, (bar>0xFF)?1:0); /* Carry */
	  }

	  registers->A = bar&0xFF;
	  /* Count 2 ticks here */
	  break;

     case 0x6A:			/* ROR */
	  baz = registers->A;
	  registers->PC++;

	  if(registers->P&0x01)
	       baz |= 0x100;

	  setbit(registers->P, 0x01, baz&0x01);
	  baz >>= 1;

	  setbit(registers->P, 0x80, baz&0x80);
	  setbit(registers->P, 0x02, (!baz)?0x02:0x00);

	  registers->A = baz;
	  /* Count 2 ticks here */
	  break;


     case 0x6B:			/* ARR */
	  foo = readmembyte(registers->PC+1);
	  baz = registers->A&foo;
	  registers->PC+=2;

	  if(registers->P&0x01)
	       baz |= 0x100;

	  setbit(registers->P, 0x01, baz&0x01);
	  baz >>= 1;

	  setbit(registers->P, 0x80, baz&0x80);
	  setbit(registers->P, 0x02, (!baz)?0x02:0x00);
	  setbit(registers->P, 0x40, (baz^registers->A)&64);

	  registers->A = baz&0xFF;
	  /* Count 2 ticks here */
	  break;

     case 0x6C:			/* JMP */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  bar = readmembyte(bar)|(readmembyte(bar)<<8);

	  registers->PC = bar;
	  /* Count 5 ticks here */
	  break;

     case 0x6D:			/* ADC */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  baz = foo+registers->A+(registers->P&0x01);
	  registers->PC+=3;

	  setbit(registers->P, 0x02, (baz == 0)?0x02:0x00);

	  if(registers->P&0x08) { /* Decimal mode */
	       if(((registers->A&0x0F)+(foo&0x0F)+(registers->P&0x01)) > 9)
		    baz+=6;

	       setbit(registers->P, 0x80, baz&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^baz)&0x80))<<6); /* Overflow */

	       if(baz > 0x99)
		    baz += 96;

	       setbit(registers->P, 0x01, (baz>0x99)?1:0); /* Carry */
	  } else {
	       setbit(registers->P, 0x80, baz&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^baz)&0x80))<<6); /* Overflow */
	       setbit(registers->P, 0x01, (baz>0xFF)?1:0); /* Carry */
	  }

	  registers->A = baz&0xFF;
	  /* Count 4 ticks here */
	  break;

     case 0x6E:			/* ROR */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;
	  
	  if(registers->P&0x01)
	       baz |= 0x100;
	  
	  setbit(registers->P, 0x01, baz&0x01);
	  baz >>= 1;

	  setbit(registers->P, 0x80, baz&0x80);
	  setbit(registers->P, 0x02, (!baz)?0x02:0x00);

	  writemembyte(bar, baz&0xFF);
	  /* Count 6 ticks here */
	  break;

     case 0x6F:			/* RRA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;

	  baz = foo >> 1;

	  if(registers->P&0x01)
	       baz |= 0x80;

	  setbit(registers->P, 0x80, baz&0x80);
	  setbit(registers->P, 0x02, (baz)?0x00:0x02);
	  setbit(registers->P, 0x40, (baz^foo)&0x40);

	  if(registers->P&0x08) {
	       if((foo&0x0F) > 4)
		    baz = (baz&0xF0)|((baz+6)&0x0F);
	       if(foo > 0x4F)
		    baz += 96;

	       setbit(registers->P, 0x01, (foo > 0x4F)?1:0);
	  } else {
	       setbit(registers->P, 0x01, (foo&0x80)?1:0);
	  }

	  writemembyte(bar, baz);
	  /* Count 6 ticks here */
	  break;

     case 0x70:			/* BVS */
	  bar = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  if(registers->P&0x40) {
	       if((*registers->PCL+bar) > 0xFF) {
		    /* Count 3 ticks here */
	       } else {
		    /* Count 4 ticks here */
	       }

	       registers->PC += bar;
	  } else {
	       /* Count 2 ticks here */
	  }
	  break;

     case 0x71:			/* ADC */
	  foo = readmembyte(registers->PC+1);
	  bar = readmembyte(foo)|(readmembyte(foo+1)<<8);
	  foo = readmembyte(bar+registers->Y);
	  baz = foo+registers->A+(registers->P&0x01);
	  registers->PC+=2;


	  if(((bar+registers->Y)&0xFF00) != (bar&0xFF00)) {
	       /* Count 6 ticks here */
	  } else {
	       /* Count 5 ticks here */
	  }
	  setbit(registers->P, 0x02, (baz == 0)?0x02:0x00);

	  if(registers->P&0x08) { /* Decimal mode */
	       if(((registers->A&0x0F)+(foo&0x0F)+(registers->P&0x01)) > 9)
		    bar+=6;

	       setbit(registers->P, 0x80, bar&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^bar)&0x80))<<6); /* Overflow */

	       if(bar > 0x99)
		    bar += 96;

	       setbit(registers->P, 0x01, (bar>0x99)?1:0); /* Carry */
	  } else {
	       setbit(registers->P, 0x80, bar&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^bar)&0x80))<<6); /* Overflow */
	       setbit(registers->P, 0x01, (bar>0xFF)?1:0); /* Carry */
	  }

	  registers->A = baz;
	  break;

     case 0x72:			/* Jam! */
	  break;

     case 0x73:			/* RRA */
	  bar = readmembyte(registers->PC+1);
	  bar = readmembyte(bar)|(readmembyte(bar)<<8);
	  foo = readmembyte(bar+registers->Y);
	  registers->PC+=2;
	  baz = foo >> 1;

	  if(registers->P&0x01)
	       baz |= 0x80;

	  setbit(registers->P, 0x80, baz&0x80);
	  setbit(registers->P, 0x02, (baz)?0x00:0x02);
	  setbit(registers->P, 0x40, (baz^foo)&0x40);

	  if(registers->P&0x08) {
	       if((foo&0x0F) > 4)
		    baz = (baz&0xF0)|((baz+6)&0x0F);
	       if(foo > 0x4F)
		    baz += 96;

	       setbit(registers->P, 0x01, (foo > 0x4F)?1:0);
	  } else {
	       setbit(registers->P, 0x01, (foo&0x80)?1:0);
	  }

	  writemembyte(bar+registers->Y, baz);
	  /* Count 8 ticks here */
	  break;

     case 0x74:			/* NOP */
	  registers->PC+=2;
	  /* Count 4 ticks here */
	  break;

     case 0x75:			/* ADC */
	  bar = readmembyte(registers->PC+1);
	  foo = readmembyte(bar+registers->X);
	  registers->PC+=2;

	  setbit(registers->P, 0x02, (baz == 0)?0x02:0x00);

	  if(registers->P&0x08) { /* Decimal mode */
	       if(((registers->A&0x0F)+(foo&0x0F)+(registers->P&0x01)) > 9)
		    bar+=6;

	       setbit(registers->P, 0x80, bar&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^bar)&0x80))<<6); /* Overflow */

	       if(bar > 0x99)
		    bar += 96;

	       setbit(registers->P, 0x01, (bar>0x99)?1:0); /* Carry */
	  } else {
	       setbit(registers->P, 0x80, bar&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^bar)&0x80))<<6); /* Overflow */
	       setbit(registers->P, 0x01, (bar>0xFF)?1:0); /* Carry */
	  }

	  registers->A = baz;
	  /* Count 4 ticks here */
	  break;

     case 0x76:			/* ROR */
	  bar = readmembyte(registers->PC+1);
	  foo = readmembyte(bar+registers->X);
	  registers->PC+=2;

	  if(registers->P&0x01)
	       baz |= 0x100;
	  
	  setbit(registers->P, 0x01, baz&0x01);
	  baz >>= 1;

	  setbit(registers->P, 0x80, baz&0x80);
	  setbit(registers->P, 0x02, (!baz)?0x02:0x00);

	  writemembyte(bar+registers->X, baz&0xFF);
	  /* Count 6 ticks here */
	  break;

     case 0x77:			/* RRA */
	  bar = readmembyte(registers->PC+1);
	  foo = readmembyte(bar+registers->X);
	  registers->PC+=2;

	  baz = foo >> 1;

	  if(registers->P&0x01)
	       baz |= 0x80;

	  setbit(registers->P, 0x80, baz&0x80);
	  setbit(registers->P, 0x02, (baz)?0x00:0x02);
	  setbit(registers->P, 0x40, (baz^foo)&0x40);

	  if(registers->P&0x08) {
	       if((foo&0x0F) > 4)
		    baz = (baz&0xF0)|((baz+6)&0x0F);
	       if(foo > 0x4F)
		    baz += 96;

	       setbit(registers->P, 0x01, (foo > 0x4F)?1:0);
	  } else {
	       setbit(registers->P, 0x01, (foo&0x80)?1:0);
	  }

	  writemembyte(bar+registers->X, baz);
	  /* Count 6 ticks here */
	  break;

     case 0x78:			/* SEI */
	  registers->PC++;

	  registers->P |= 0x04;
	  /* Count 2 ticks here */
	  break;

     case 0x79:			/* ADC */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->Y);
	  baz = foo+registers->A+(registers->P&0x01);
	  registers->PC+=3;

	  if(((bar+registers->Y)&0xFF00) != (bar&0xFF00)) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }
	  setbit(registers->P, 0x02, (baz == 0)?0x02:0x00);

	  if(registers->P&0x08) { /* Decimal mode */
	       if(((registers->A&0x0F)+(foo&0x0F)+(registers->P&0x01)) > 9)
		    baz+=6;

	       setbit(registers->P, 0x80, baz&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^baz)&0x80))<<6); /* Overflow */

	       if(baz > 0x99)
		    baz += 96;

	       setbit(registers->P, 0x01, (baz>0x99)?1:0); /* Carry */
	  } else {
	       setbit(registers->P, 0x80, baz&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^baz)&0x80))<<6); /* Overflow */
	       setbit(registers->P, 0x01, (baz>0xFF)?1:0); /* Carry */
	  }

	  registers->A = baz&0xFF;
	  break;

     case 0x7A:			/* NOP */
	  registers->PC++;
	  /* Count 2 ticks here */
	  break;

     case 0x7B:			/* RRA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->Y);
	  registers->PC+=3;

	  baz = foo >> 1;

	  if(registers->P&0x01)
	       baz |= 0x80;

	  setbit(registers->P, 0x80, baz&0x80);
	  setbit(registers->P, 0x02, (baz)?0x00:0x02);
	  setbit(registers->P, 0x40, (baz^foo)&0x40);

	  if(registers->P&0x08) {
	       if((foo&0x0F) > 4)
		    baz = (baz&0xF0)|((baz+6)&0x0F);
	       if(foo > 0x4F)
		    baz += 96;

	       setbit(registers->P, 0x01, (foo > 0x4F)?1:0);
	  } else {
	       setbit(registers->P, 0x01, (foo&0x80)?1:0);
	  }

	  writemembyte(bar+registers->Y, baz);
	  /* Count 7 ticks here */
	  break;

     case 0x7C:			/* NOP */
	  registers->PC+=3;
	  /* Count 4 ticks here */
	  break;

     case 0x7D:			/* ADC */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->X);
	  baz = foo+registers->A+(registers->P&0x01);
	  registers->PC+=3;

	  if(((bar+registers->X)&0xFF00) != (bar&0xFF00)) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }
	  setbit(registers->P, 0x02, (baz == 0)?0x02:0x00);

	  if(registers->P&0x08) { /* Decimal mode */
	       if(((registers->A&0x0F)+(foo&0x0F)+(registers->P&0x01)) > 9)
		    bar+=6;

	       setbit(registers->P, 0x80, bar&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^bar)&0x80))<<6); /* Overflow */

	       if(bar > 0x99)
		    bar += 96;

	       setbit(registers->P, 0x01, (bar>0x99)?1:0); /* Carry */
	  } else {
	       setbit(registers->P, 0x80, bar&0x80); /* Sign */
	       setbit(registers->P, 0x40, (!((registers->A^foo)&0x80) &&
				((registers->A^bar)&0x80))<<6); /* Overflow */
	       setbit(registers->P, 0x01, (bar>0xFF)?1:0); /* Carry */
	  }

	  registers->A = baz;
	  break;

     case 0x7E:			/* ROR */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->X);
	  registers->PC+=3;

	  if(registers->P&0x01)
	       baz |= 0x100;
	  
	  setbit(registers->P, 0x01, baz&0x01);
	  baz >>= 1;

	  setbit(registers->P, 0x80, baz&0x80);
	  setbit(registers->P, 0x02, (!baz)?0x02:0x00);

	  writemembyte(bar+registers->X, baz&0xFF);
	  /* Count 7 ticks here */
	  break;

     case 0x7F:			/* RRA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->X);
	  registers->PC+=3;

	  baz = foo >> 1;

	  if(registers->P&0x01)
	       baz |= 0x80;

	  setbit(registers->P, 0x80, baz&0x80);
	  setbit(registers->P, 0x02, (baz)?0x00:0x02);
	  setbit(registers->P, 0x40, (baz^foo)&0x40);

	  if(registers->P&0x08) {
	       if((foo&0x0F) > 4)
		    baz = (baz&0xF0)|((baz+6)&0x0F);
	       if(foo > 0x4F)
		    baz += 96;

	       setbit(registers->P, 0x01, (foo > 0x4F)?1:0);
	  } else {
	       setbit(registers->P, 0x01, (foo&0x80)?1:0);
	  }

	  writemembyte(bar+registers->X, baz);
	  /* Count 7 ticks here */
	  break;

     case 0x80:			/* NOP */
	  registers->PC+=2;
	  /* Count 2 ticks here */
	  break;

     case 0x81:			/* STA */
	  bar = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  writemembyte(readmembyte(bar+registers->X)|
		       (readmembyte(bar+registers->X+1)<<8),
		       registers->A);
	  /* Count 6 ticks here */
	  break;

     case 0x82:			/* NOP/Jam */
	  registers->PC+=2;
	  /* Count 2 ticks here */
	  break;

     case 0x83:			/* SAX */
	  bar = readmembyte(registers->PC+1);
	  foo = registers->A&registers->X;
	  registers->PC+=2;

	  writemembyte(readmembyte(bar+registers->X)|
		       (readmembyte(bar+registers->X)<<8), foo);
	  /* Count 6 ticks here */
	  break;

     case 0x84:			/* STY */
	  bar = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  writemembyte(bar, registers->Y);
	  /* Count 3 ticks here */
	  break;

     case 0x85:			/* STA */
	  bar = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  writemembyte(bar, registers->A);
	  /* Count 3 ticks here */
	  break;

     case 0x86:			/* STX */
	  bar = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  writemembyte(bar, registers->X);
	  /* Count 3 ticks here */
	  break;

     case 0x87:			/* SAX */
	  bar = readmembyte(registers->PC+1);
	  foo = registers->A&registers->X;
	  registers->PC+=2;

	  writemembyte(bar, foo);
	  /* Count 3 ticks here */
	  break;

     case 0x88:			/* DEY */
	  registers->PC++;

	  registers->Y--;
	  setbit(registers->P, 0x080, registers->Y&0x80);
	  setbit(registers->P, 0x02, (!registers->Y)?0x02:0x00);

	  /* Count 2 ticks here */
	  break;

     case 0x89:			/* NOP */
	  registers->PC+=2;
	  /* Count 2 ticks here */
	  break;

     case 0x8A:			/* TXA */
	  registers->PC++;
	  setbit(registers->P, 0x080, registers->X&0x80);
	  setbit(registers->P, 0x02, (!registers->X)?0x02:0x00);
	  registers->A = registers->X;
	  /* Count 2 ticks here */
	  break;

     case 0x8B:			/* ANE */
	  bar = readmembyte(registers->PC+1);
	  foo = ((registers->A|0xEE)&registers->X&bar);
	  registers->PC += 2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x00);

	  registers->A = foo;
	  /* Count 2 ticks here */
	  break;

     case 0x8C:			/* STY */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;

	  writemembyte(bar, registers->Y);
	  /* Count 4 ticks here */
	  break;

     case 0x8D:			/* STA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;

	  writemembyte(bar, registers->A);
	  /* Count 4 ticks here */
	  break;

     case 0x8E:			/* STX */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;

	  writemembyte(bar, registers->X);
	  /* Count 4 ticks here */
	  break;

     case 0x8F:			/* SAX */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;

	  writemembyte(bar, registers->A&registers->X);
	  /* Count 4 ticks here */
	  break;

     case 0x90:			/* BCC */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  if(registers->P&0x01) {
	       if(((registers->PC+foo)&0xFF00) != (registers->PC&0xFF00)) {
		    /* Count 3 ticks here */
	       } else {
		    /* Count 4 ticks here */
	       }
	       registers->PC += foo;
	  } else {
	       /* Count 2 ticks here */
	  }
	  break;

     case 0x91:			/* STA */
	  bar = readmembyte(registers->PC+1);
	  bar = readmembyte(bar)|(readmembyte(bar+1)<<8);
	  registers->PC+=2;

	  writemembyte(bar+registers->Y, registers->A);
	  /* Count 6 ticks here */
	  break;

     case 0x92:			/* Jam! */
	  break;

     case 0x93:			/* SHA */
	  bar = readmembyte(registers->PC+1);
	  bar = readmembyte(bar)|(readmembyte(bar+1)<<8);
	  foo = registers->A&registers->X;
	  registers->PC+=2;

	  foo &= ((bar>>8)+1)&0xFF;
	  if((bar&0xFF)+registers->Y > 0xFF)
	       bar = foo;
	  writemembyte(bar+registers->Y, foo);
	  /* Count 6 cycles here */
	  break;

     case 0x94:			/* STY */
	  bar = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  writemembyte(bar+registers->X, registers->Y);
	  /* Count 4 ticks here */
	  break;

     case 0x95:			/* STA */
	  bar = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  writemembyte(bar+registers->X, registers->A);
	  /* Count 4 ticks here */
	  break;

     case 0x96:			/* STX */
	  bar = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  writemembyte(bar+registers->Y, registers->X);
	  /* Count 4 ticks here */
	  break;

     case 0x97:			/* SAX */
	  bar = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  writemembyte(bar+registers->Y, registers->A&registers->X);
	  /* Count 4 ticks here */
	  break;

     case 0x98:			/* TYA */
	  registers->PC++;

	  setbit(registers->P, 0x080, registers->Y&0x80);
	  setbit(registers->P, 0x02, (!registers->Y)?0x02:0x00);

	  registers->A = registers->Y;
	  /* Count 2 ticks here */
	  break;

     case 0x99:			/* STA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;

	  writemembyte(bar+registers->Y, registers->A);
	  /* Count 5 ticks here */
	  break;

     case 0x9A:			/* TXS */
	  registers->PC++;

	  registers->S = registers->X;
	  /* Count 2 ticks here */
	  break;

     case 0x9B:			/* SHS */
	  registers->PC+=3;

	  registers->S = registers->A&registers->X;
	  /* Count 5 ticks here */
	  break;

     case 0x9C:			/* SHY */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;
	  foo = registers->Y;

	  foo &= (((bar >> 8)+1)&0xff);
	  if (((bar&0xff)+registers->X) > 0xff)
	       bar = foo;
	  writemembyte(bar+registers->X, foo);
	  /* Count 5 ticks here */
	  break;

     case 0x9D:			/* STA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;

	  writemembyte(bar+registers->X, registers->A);
	  /* Count 5 ticks here */
	  break;

     case 0x9E:			/* SHX */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;
	  foo = registers->X;

	  foo &= (((bar >> 8)+1)&0xff);
	  if (((bar&0xff)+registers->Y) > 0xff)
	       bar = foo;
	  writemembyte(bar+registers->Y, foo);
	  /* Count 5 ticks here */
	  break;

     case 0x9F:			/* SHA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  registers->PC+=3;
	  foo = registers->A&registers->X;

	  foo &= (((bar >> 8)+1)&0xff);
	  if (((bar&0xff)+registers->Y) > 0xff)
	       bar = foo;
	  writemembyte(bar+registers->Y, foo);
	  /* Count 5 ticks here */
	  break;

     case 0xA0:			/* LDY */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->Y = foo;
	  /* Count 2 ticks here */
	  break;

     case 0xA1:			/* LDA */
	  foo = readmembyte(registers->PC+1);
	  bar = readmembyte(foo+registers->X)|
	       (readmembyte(foo+1+registers->X)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  /* Count 6 ticks here */
	  break;

     case 0xA2:			/* LDX */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->X = foo;
	  /* Count 2 ticks here */
	  break;

     case 0xA3:			/* LAX */
	  foo = readmembyte(registers->PC+1);
	  bar = readmembyte(foo+registers->X)|
	       (readmembyte(foo+1+registers->X)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  registers->X = foo;
	  /* Count 6 ticks here */
	  break;

     case 0xA4:			/* LDY */
	  foo = readmembyte(readmembyte(registers->PC+1));
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->Y = foo;
	  /* Count 3 ticks here */
	  break;

     case 0xA5:			/* LDA */
	  foo = readmembyte(readmembyte(registers->PC+1));
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  /* Count 3 ticks here */
	  break;

     case 0xA6:			/* LDX */
	  foo = readmembyte(readmembyte(registers->PC+1));
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->X = foo;
	  /* Count 3 ticks here */
	  break;

     case 0xA7:			/* LAX */
	  foo = readmembyte(readmembyte(registers->PC+1));
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  registers->X = foo;
	  /* Count 3 ticks here */
	  break;

     case 0xA8:			/* TAY */
	  registers->PC++;

	  setbit(registers->P, 0x80, registers->A&0x80);
	  setbit(registers->P, 0x02, (!registers->A)?0x02:0x0);

	  registers->Y = registers->A;
	  /* Count 2 ticks here */
	  break;

     case 0xA9:			/* LDA */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  /* Count 2 ticks here */
	  break;

     case 0xAA:			/* TAX */
	  registers->PC++;

	  setbit(registers->P, 0x80, registers->A&0x80);
	  setbit(registers->P, 0x02, (!registers->A)?0x02:0x0);

	  registers->X = registers->A;
	  /* Count 2 ticks here */
	  break;

     case 0xAB:			/* LXA */
	  foo = registers->A&readmembyte(registers->PC+1);
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  registers->X = foo;
	  /* Count 2 ticks here */
	  break;

     case 0xAC:			/* LDY */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->Y = foo;
	  /* Count 4 ticks here */
	  break;

     case 0xAD:			/* LDA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  /* Count 4 ticks here */
	  break;

     case 0xAE:			/* LDX */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->X = foo;
	  /* Count 4 ticks here */
	  break;

     case 0xAF:			/* LAX */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  registers->X = foo;
	  /* Count 4 ticks here */
	  break;

     case 0xB0:			/* BCS */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  if(registers->P&0x01) {
	       if(((registers->PC+foo)&0xff00) != (registers->PC&0xff00)) {
		    /* Count 3 ticks here */
	       } else {
		    /* Count 4 ticks here */
	       }
	       registers->PC+=foo;
	  } else {
	       /* Count 2 ticks here */
	  }
	  break;

     case 0xB1:			/* LDA */
	  foo = readmembyte(registers->PC+1);
	  bar = readmembyte(foo)|(readmembyte(foo+1)<<8);
	  foo = readmembyte(bar+registers->Y);
	  registers->PC+=2;

	  if(((bar+registers->Y)&0xff00) != (bar&0xff00)) {
	       /* Count 6 ticks here */
	  } else {
	       /* Count 5 ticks here */
	  }

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  break;

     case 0xB2:			/* Jam! */
	  break;

     case 0xB3:			/* LAX */
	  foo = readmembyte(registers->PC+1);
	  bar = readmembyte(foo)|(readmembyte(foo+1)<<8);
	  foo = readmembyte(bar+registers->Y);
	  registers->PC+=2;

	  if(((bar+registers->Y)&0xff00) != (bar&0xff00)) {
	       /* Count 6 ticks here */
	  } else {
	       /* Count 5 ticks here */
	  }

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  registers->X = foo;
	  break;

     case 0xB4:			/* LDY */
	  foo = readmembyte(readmembyte(registers->PC+1)+registers->X);
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->Y = foo;
	  /* Count 4 ticks here */
	  break;

     case 0xB5:			/* LDA */
	  foo = readmembyte(readmembyte(registers->PC+1)+registers->X);
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  /* Count 4 ticks here */
	  break;

     case 0xB6:			/* LDX */
	  foo = readmembyte(readmembyte(registers->PC+1)+registers->Y);
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->X = foo;
	  /* Count 4 ticks here */
	  break;

     case 0xB7:			/* LAX */
	  foo = readmembyte(readmembyte(registers->PC+1)+registers->Y);
	  registers->PC+=2;

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  registers->X = foo;
	  /* Count 4 ticks here */
	  break;

     case 0xB8:			/* CLV */
	  registers->PC++;
	  registers->P &= ~0x40;
	  /* Count 2 ticks here */
	  break;

     case 0xB9:			/* LDA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->Y);
	  registers->PC+=3;

	  if(((bar+registers->Y)&0xff00) != (bar&0xff00)) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }

	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  break;

     case 0xBA:			/* TSX */
	  registers->PC++;

	  setbit(registers->P, 0x080, registers->S&0x80);
	  setbit(registers->P, 0x02, (!registers->S)?0x02:0x0);

	  registers->X = registers->S;
	  /* Count 2 ticks here */
	  break;

     case 0xBB:			/* LAS */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = registers->S&readmembyte(bar+registers->Y);
	  registers->PC+=3;

	  if((bar&0xFF00) != ((bar+registers->Y)&0xFF00)) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }
	  
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = registers->X = registers->S = foo;
	  break;

     case 0xBC:			/* LDY */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(registers->X+bar);
	  registers->PC+=3;

	  if((bar&0xFF00) != ((bar+registers->X)&0xFF00)) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }
	  
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->Y = foo;
	  break;

     case 0xBD:			/* LDA */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(registers->X+bar);
	  registers->PC+=3;

	  if((bar&0xFF00) != ((bar+registers->X)&0xFF00)) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }
	  
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->A = foo;
	  break;

     case 0xBE:			/* LDX */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(registers->Y+bar);
	  registers->PC+=3;

	  if((bar&0xFF00) != ((bar+registers->Y)&0xFF00)) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }
	  
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers->X = foo;
	  break;

     case 0xBF:			/* LAX */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(registers->Y+bar);
	  registers->PC+=3;

	  if((bar&0xFF00) != ((bar+registers->Y)&0xFF00)) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }
	  
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (!foo)?0x02:0x0);

	  registers-> A = registers->X = foo;
	  break;

     case 0xC0:			/* CPY */
	  foo = readmembyte(registers->PC+1);
	  registers->PC += 2;

	  bar = registers->Y-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  /* Count 2 ticks here */
	  break;

     case 0xC1:			/* CMP */
	  foo = readmembyte(registers->PC+1);
	  foo = readmembyte(readmembyte(foo+registers->X) +
			    readmembyte(foo+1+registers->X));
	  registers->PC += 2;

	  bar = registers->A-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  /* Count 6 ticks here */	  
	  break;

     case 0xC2:			/* NOP */
	  registers->PC+=2;
	  /* Count 2 ticks here */
	  break;

     case 0xC3:			/* DCP */
	  foo = readmembyte(registers->PC+1);
	  bar = readmembyte(foo+registers->X) +
	       readmembyte(foo+1+registers->X);
	  foo = readmembyte(bar);
	  registers->PC += 2;

	  foo--;
	  setbit(registers->P, 0x01, (registers->A >= foo));
	  setbit(registers->P, 0x80, (registers->A-foo)&0x80);
	  setbit(registers->P, 0x02, (registers->A != foo));

	  writemembyte(bar, foo);
	  /* Count 8 ticks here */	  
	  break;

     case 0xC4:			/* CPY */
	  foo = readmembyte(readmembyte(registers->PC+1));
	  registers->PC += 2;

	  bar = registers->Y-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  /* Count 3 ticks here */	  
	  break;

     case 0xC5:			/* CMP */
	  foo = readmembyte(readmembyte(registers->PC+1));
	  registers->PC += 2;

	  bar = registers->A-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  /* Count 3 ticks here */	  
	  break;

     case 0xC6:			/* DEC */
	  bar = readmembyte(registers->PC+1);
	  foo = readmembyte(bar);
	  registers->PC += 2;

	  foo--;
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (foo &= 0xFF));

	  writemembyte(bar, foo);
	  /* Count 5 ticks here */
	  break;

     case 0xC7:			/* DCP */
	  bar = readmembyte(registers->PC+1);
	  foo = readmembyte(bar);
	  registers->PC += 2;

	  foo--;
	  setbit(registers->P, 0x01, (registers->A >= foo));
	  setbit(registers->P, 0x80, (registers->A-foo)&0x80);
	  setbit(registers->P, 0x02, (registers->A != foo));

	  writemembyte(bar, foo);
	  /* Count 5 ticks here */	  
	  break;

     case 0xC8:			/* INY */
	  registers->PC++;

	  registers->Y++;
	  setbit(registers->P, 0x80, registers->Y&0x80);
	  setbit(registers->P, 0x02, (!registers->Y)<<1);

	  /* Count 2 ticks here */
	  break;

     case 0xC9:			/* CMP */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  bar = registers->A-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  /* Count 2 ticks here */	  
	  break;

     case 0xCA:			/* DEX */
	  registers->PC++;

	  registers->X--;
	  setbit(registers->P, 0x80, registers->X&0x80);
	  setbit(registers->P, 0x02, (!registers->X)<<1);

	  /* Count 2 ticks here */
	  break;

     case 0xCB:			/* SBX */
	  foo = readmembyte(registers->PC + 1);
	  registers->PC += 2;

	  bar = (registers->A & registers->X) - foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  registers->X = bar;
	  /* Count 2 ticks here */
	  break;

     case 0xCC:			/* CPY */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;

	  bar = registers->Y-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  /* Count 4 ticks here */	  
	  break;

     case 0xCD:			/* CMP */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;

	  bar = registers->A-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  /* Count 4 ticks here */	  
	  break;

     case 0xCE:			/* DEC */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;

	  foo--;
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (foo &= 0xFF));

	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;

     case 0xCF:			/* DCP */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;

	  foo--;
	  setbit(registers->P, 0x01, (registers->A >= foo));
	  setbit(registers->P, 0x80, (registers->A-foo)&0x80);
	  setbit(registers->P, 0x02, (registers->A != foo));

	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;

     case 0xD0:			/* BNE */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;

	  if(registers->P&0x02){ 
	       /* Count 2 ticks here */
	  } else {
	       if(((registers->PC+foo)&0xff) != (registers->PC&0xff)) {
		    /* Count 4 ticks here */
	       } else {
		    /* Count 3 ticks here */
	       }
	       registers->PC+=foo;
	  }

     case 0xD1:			/* CMP */
	  foo = readmembyte(registers->PC+1);
	  bar = (readmembyte(foo)|(readmembyte(foo)<<8));
	  foo = readmembyte(bar+registers->Y);
	  registers->PC += 2;

	  if(((bar+registers->Y)&0xff) != (bar&0xff)) {
	       /* Count 6 ticks here */
	  } else {
	       /* Count 5 ticks here */
	  }

	  bar = registers->A-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  break;

     case 0xD2:			/* Jam! */
	  break;

     case 0xD3:			/* DCP */
	  foo = readmembyte(registers->PC+1);
	  bar = readmembyte((readmembyte(foo)|(readmembyte(foo)<<8))+
			    registers->Y);
	  registers->PC += 2;

	  foo--;
	  setbit(registers->P, 0x01, (registers->A >= foo));
	  setbit(registers->P, 0x80, (registers->A-foo)&0x80);
	  setbit(registers->P, 0x02, (registers->A != foo));

	  writemembyte(bar, foo);
	  /* Count 8 ticks here */
	  break;

     case 0xD4:			/* NOP */
	  registers->PC += 2;

	  /* Count 4 ticks here */
	  break;

     case 0xD5:			/* CMP */
	  bar = readmembyte(registers->PC+1)+registers->X;
	  foo = readmembyte(bar);
	  registers->PC += 2;

	  bar = registers->A-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  /* Count 4 ticks here */
	  break;

     case 0xD6:			/* DEC */
	  bar = readmembyte(registers->PC+1)+registers->X;
	  foo = readmembyte(bar);
	  registers->PC += 2;

	  foo--;
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (foo &= 0xFF));

	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;

     case 0xD7:			/* DCP */
	  bar = readmembyte(registers->PC+1)+registers->X;
	  foo = readmembyte(bar);
	  registers->PC += 2;
	  
	  foo--;
	  setbit(registers->P, 0x01, (registers->A >= foo));
	  setbit(registers->P, 0x80, (registers->A-foo)&0x80);
	  setbit(registers->P, 0x02, (registers->A != foo));

	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;

     case 0xD8:			/* CLD */
	  registers->PC++;

	  setbit(registers->P, 0x08, 0);
	  /* Count 2 ticks here */
	  break;

     case 0xD9:			/* CMP */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->Y);
	  registers->PC+=3;

	  if(((bar+registers->Y)&0xff) != (bar&0xff)) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }

	  bar = registers->A-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  break;

     case 0xDA:			/* NOP */
	  registers->PC++;

	  /* Count 2 ticks here */
	  break;

     case 0xDB:			/* DCP */
	  bar = (readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8))+registers->Y;
	  foo = readmembyte(bar);
	  registers->PC+=3;

	  foo--;
	  setbit(registers->P, 0x01, (registers->A >= foo));
	  setbit(registers->P, 0x80, (registers->A-foo)&0x80);
	  setbit(registers->P, 0x02, (registers->A != foo));

	  writemembyte(bar, foo);
	  /* Count 7 ticks here */
	  break;

     case 0xDC:			/* NOP */
	  registers->PC += 3;

	  /* Count 4 ticks here */
	  break;

     case 0xDD:			/* CMP */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar+registers->X);
	  registers->PC+=3;

	  if(((bar+registers->X)&0xff) != (bar&0xff)) {
	       /* Count 5 ticks here */
	  } else {
	       /* Count 4 ticks here */
	  }

	  bar = registers->A-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  break;

     case 0xDE:			/* DEC */
	  bar = (readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8))+registers->X;
	  foo = readmembyte(bar);
	  registers->PC += 3;

	  foo--;
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (foo &= 0xFF));

	  writemembyte(bar, foo);
	  /* Count 7 ticks here */
	  break;

     case 0xDF:			/* DCP */
	  bar = (readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8))+registers->X;
	  foo = readmembyte(bar);
	  registers->PC += 3;

	  foo--;
	  setbit(registers->P, 0x01, (registers->A >= foo));
	  setbit(registers->P, 0x80, (registers->A-foo)&0x80);
	  setbit(registers->P, 0x02, (registers->A != foo));

	  writemembyte(bar, foo);
	  /* Count 7 ticks here */
	  break;

     case 0xE0:			/* CPX */
	  foo = readmembyte(registers->PC+1);
	  registers->PC += 2;

	  bar = registers->X-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  /* Count 2 ticks here */
	  break;

     case 0xE1:			/* SBC */
	  foo = readmembyte(registers->PC+1);
	  registers->PC+=2;
	  registers->A -= readmembyte((word_t)(readmembyte(registers->X+foo)|
				      (readmembyte((word_t)
						   (registers->X+foo+
						    (byte_t)1))<<8)));
	  setbit(registers->P, 0x80, registers->A&0x80); /* Sign */
	  setbit(registers->P, 0x02,
		 (registers->A == (byte_t)0)<<(unsigned)1); /* Zero */
	  /* Count 6 ticks here */
	  break;

     case 0xE2:			/* NOP */
	  registers->PC += 2;

	  /* Count 2 ticks here */
	  break;

     case 0xE3:			/* ISB */
	  /* I'm not sure how to do ISB */
	  break;

     case 0xE4:			/* CPX */
	  foo = readmembyte(readmembyte(registers->PC+1));
	  registers->PC += 2;

	  bar = registers->X-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  /* Count 3 ticks here */
	  break;

     case 0xE5:			/* SBC */
	  /* I'm not sure how to do SBC */
	  break;

     case 0xE6:			/* INC */
	  foo = readmembyte((bar = readmembyte(registers->PC+1)));
	  registers->PC += 2;

	  foo++;
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (foo &= 0xFF));

	  writemembyte(bar, foo);
	  /* Count 5 ticks here */
	  break;

     case 0xE7:			/* ISB */
	  /* I'm not sure how to do ISB */
	  break;

     case 0xE8:			/* INX */
	  registers->PC++;

	  registers->X++;
	  setbit(registers->P, 0x80, registers->X&0x80);
	  setbit(registers->P, 0x02, (registers->X &= 0xFF));

	  /* Count 2 ticks here */
	  break;

     case 0xE9:			/* SBC */
	  /* I'm not sure how to do SBC */
	  break;

     case 0xEA:			/* NOP */
	  registers->PC++;

	  /* Count 2 ticks here */
	  break;

     case 0xEB:			/* SBC */
	  /* I'm not sure how to do SBC */
	  break;

     case 0xEC:			/* CPX */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;

	  bar = registers->X-foo;
	  setbit(registers->P, 0x01, (bar < 0x100));
	  setbit(registers->P, 0x80, bar&0x80);
	  setbit(registers->P, 0x02, (bar &= 0xFF));

	  /* Count 4 ticks here */
	  break;

     case 0xED:			/* SBC */
	  /* I'm not sure how to do SBC */
	  break;

     case 0xEE:			/* INC */
	  bar = readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8);
	  foo = readmembyte(bar);
	  registers->PC+=3;

	  foo++;
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (foo &= 0xFF));

	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;

     case 0xEF:			/* ISB */
	  /* I'm not sure how to do ISB */
	  break;

     case 0xF0:			/* BEQ */
	  foo = readmembyte(registers->PC+1);
	  registers->PC += 2;

	  if(registers->P&0x02) {
	       if(((registers->PC+foo)&0xff) != (registers->PC&0xff)) {
		    /* Count 4 ticks here */
	       } else {
		    /* Count 3 ticks here */
	       }
	       registers->PC+=foo;
	  } else {
	       /* Count 2 ticks here */
	  }
	  break;

     case 0xF1:			/* SBC */
	  /* I'm not sure how to do SBC */
	  break;

     case 0xF2:			/* Jam! */
	  break;

     case 0xF3:			/* ISB */
	  /* I'm not sure how to do ISB */
	  break;

     case 0xF4:			/* NOP */
	  registers->PC += 2;

	  /* Count 4 ticks here */
	  break;

     case 0xF5:			/* SBC */
	  /* I'm not sure how to do SBC */
	  break;

     case 0xF6:			/* INC */
	  bar = readmembyte(registers->PC+1)+registers->X;
	  foo = readmembyte(bar);
	  registers->PC += 2;

	  foo++;
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (foo &= 0xFF));

	  writemembyte(bar, foo);
	  /* Count 6 ticks here */
	  break;

     case 0xF7:			/* ISB */
	  /* I'm not sure how to do ISB */
	  break;

     case 0xF8:			/* SED */
	  registers->PC++;

	  registers->P |= 0x08;
	  /* Count 2 ticks here */
	  break;

     case 0xF9:			/* SBC */
	  /* I'm not sure how to do SBC */
	  break;

     case 0xFA:			/* NOP */
	  registers->PC++;

	  /* Count 2 ticks here */
	  break;

     case 0xFB:			/* ISB */
	  /* I'm not sure how to do ISB */
	  break;

     case 0xFC:			/* NOP */
	  registers->PC += 3;

	  /* Count 4 ticks here */
	  break;

     case 0xFD:			/* SBC */
	  /* I'm not sure how to do SBC */
	  break;

     case 0xFE:			/* INC */
	  bar = (readmembyte(registers->PC+1)|
	       (readmembyte(registers->PC+2)<<8))+registers->X;
	  foo = readmembyte(bar);
	  registers->PC+=3;

	  foo++;
	  setbit(registers->P, 0x80, foo&0x80);
	  setbit(registers->P, 0x02, (foo &= 0xFF));

	  writemembyte(bar, foo);
	  /* Count 7 ticks here */
	  break;

     case 0xFF:			/* ISB */
	  /* I'm not sure how to do ISB */
	  break;

	  /* No other values could occur */
     default:			/* ... */
	  break;

     }
     return;
}

/* EOF 6502core.c */
