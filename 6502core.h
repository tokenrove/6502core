#ifndef _6502CORE_H
#define _6502CORE_H

#define setbit(a, b, c) (a &= ~b); (a |= (c))

typedef unsigned short word_t;
typedef unsigned char byte_t;

typedef struct registers_t {
     word_t PC;			/* Program Counter */
     byte_t *PCH, *PCL;		/* PC High and Low */
     byte_t S;			/* Stack pointer */
     byte_t P;			/* Processor status */
     byte_t A;			/* Accumulator */
     byte_t X, Y;		/* Index registers */
} registers_t;

#endif /* !_6502CORE_H */
