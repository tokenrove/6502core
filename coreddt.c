/* 
 * coreddt.c
 * Created: Wed Aug  4 20:41:36 1999 by tek@wiw.org
 * Revised: Wed Aug  4 21:54:21 1999 by tek@wiw.org
 * Copyright 1999 Julian E. C. Squires (tek@wiw.org)
 * This program comes with ABSOLUTELY NO WARRANTY.
 * $Id$
 * 
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "6502core.h"


#define BUFSIZE 128
#define MEMSIZE 65536

byte_t memory[MEMSIZE];

byte_t readmembyte(word_t address);
void writemembyte(word_t address, byte_t data);
void *xmemset(void *, char value, size_t size);

int main(int argc, char **argv)
{
    registers_t *R = (registers_t *)malloc(sizeof(registers_t));
    FILE *fp;
    char buf[BUFSIZE], *p;
    int org;

    setupmachine(R);
    
    while(!feof(stdin))
    {
	printf("> "); fflush(stdout);
	fgets(buf, BUFSIZE-1, stdin);
	buf[strlen(buf)-1] = 0;

	p = strtok(buf, " ");
	if(strcmp(p, "quit") == 0) {
	    printf("\nShutting down.\n");
	    break;

	} else if(strcmp(p, "step") == 0) {
	    cpustep(R);

	} else if(strcmp(p, "org") == 0) {
	    p = strtok(NULL, " ");
	    if(p == NULL) {
		printf("%04X\n", org);
		continue;
	    }
	    org = atoi(p);

	} else if(strcmp(p, "load") == 0) {
	    p = strtok(NULL, " ");
	    if(p == NULL) { continue; }
	    fp = fopen(p, "r");
	    if(errno) {
		perror(p);
		exit(EXIT_FAILURE);
	    }
	    fread(memory+org, sizeof(byte_t), MEMSIZE-org, fp);

        } else if(strcmp(p, "help") == 0) {
	    printf("quit\n");
	    printf("step\n");
	    printf("load\n");
	    printf("help\n");
	    printf("regs\n");
	    printf("org\n");

	} else if(strcmp(p, "regs") == 0) {
	    printf("A: %04X  X: %02X  Y: %02X  PC: %04X\n", R->A, R->X, R->Y,
		   R->PC);
	    
	    if(R->P&0x80) { printf("N"); }
	    if(R->P&0x40) { printf("V"); }
	    if(R->P&0x20) { printf("1"); }
	    if(R->P&0x10) { printf("B"); }
	    if(R->P&0x08) { printf("D"); }
	    if(R->P&0x04) { printf("I"); }
	    if(R->P&0x02) { printf("Z"); }
	    if(R->P&0x01) { printf("C"); }

	    printf("\n");

	} else if(p[0] == 0) {
	} else {
	    printf("Bad command. `help' for help.\n");
        }
    }
    
    free(R);
    exit(EXIT_SUCCESS);
}

byte_t readmembyte(word_t address)
{
    return memory[address];
}

void writemembyte(word_t address, byte_t data)
{
    memory[address] = data;
}

void *xmemset(void *m, char value, size_t size)
{
    return memset(m, value, size);
}

/* EOF coreddt.c */
