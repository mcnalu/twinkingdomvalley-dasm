#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define UCHAR unsigned char

#define FILESIZE 23040
#define MAXWORD 100
#define MAXLINE 1000
#define NOBJWORDS 4
#define NNPCWORDS 3

#define NOTHING 0
#define L14CE 1
#define L3B6C 2

//The start address of TKV code in BBC's memory 
long start;
//ALL other addresses have start subtracted from them in this code.
long lbwordtable,hbwordtable;
long lbloctable,hbloctable;

long offsettable;

//TKV code as re-ordered into BBC's memory at very start of game.
unsigned char ctkv[FILESIZE];

void init_tkv();
UCHAR getbyte(char *addrs, UCHAR  y);
UCHAR getnumberofobjects();
UCHAR getnumberofcharacters();
void printdirectiondescription(char *word, UCHAR dirbyte);
void printobjectdescription(char *w, UCHAR code);
void printcharacterdescription(UCHAR *line, UCHAR classcode);
long getlocationaddress(unsigned char code);
long getaddressfromtable(long tablelb, long tablehb,unsigned char code);
void namelocation(char *name, long addr);
void incodedescription(char *ss, UCHAR b);
void incodeexits(char *ss, UCHAR b);
long getcommandaddress(UCHAR code);
void getword(char *w, UCHAR code);
long getwordforaddress(char *word,long address);
