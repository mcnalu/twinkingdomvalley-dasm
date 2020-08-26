#include <ctype.h>
#include "libtkv.h"

#define NLOCATIONS 2

struct location {
  UCHAR id;
  char *description;
  UCHAR nexits;
  struct exit *exits;
  UCHAR isdark;
  UCHAR islongdesc;
  UCHAR isvisited;
};

struct exit {
  UCHAR direction;
  char *dirtext;
  UCHAR destination;
  UCHAR ispassable;
  UCHAR exittype;
  UCHAR locktype;
  UCHAR isopen;
};

char * copytolower(char *from);
void load_location(struct location *l, UCHAR id);
long load_exit(struct exit *e, long addr);
char * get_dirtext(UCHAR dirbyte);
void free_location(struct location *l);
void print_short_description(struct location *locations, int i);
void print_long_description(struct location *locations, int i);


int main(int argc, char *argv[]) {
  init_tkv();
  int i;
  struct location *locations = (struct location *) malloc(NLOCATIONS*sizeof(struct location));
  for(i=0;i<NLOCATIONS;i++){
    load_location(locations+i,i);
    print_short_description(locations,i);
  }
  print_long_description(locations,0);
  print_long_description(locations,1);
  for(i=0;i<NLOCATIONS;i++)
    free_location(locations+i);
  free(locations);
}

//Copys string, dropping case and creating memory for to string - remember to free it!
char * copytolower(char *from){
  int len=strlen(from);
  char *to=(char *) malloc(len+1);
  while(len>=0){//Copy string and drop to lower case
    to[len]=tolower(from[len]);
    len--;
  }
  return to;
}

void load_location(struct location *l, UCHAR id){
  long addr=getlocationaddress(id);
  char desc[MAXLINE];
  int i;
  l->id=id;
  //Set up the description
  namelocation(desc,addr);
  l->description=copytolower(desc);
  //Work out number of exits
  UCHAR b7A=c[addr+1];
  UCHAR  nwords=b7A&0x1F;
  l->nexits=b7A>>5;
  l->exits = (struct exit *) malloc(l->nexits*sizeof(struct exit));
  addr+=2+nwords;  //Address for direction/destination bytes
  for(i=0;i<l->nexits;i++){
    addr=load_exit((l->exits)+i,addr);
  }
}

long load_exit(struct exit *e, long addr){
    UCHAR  first = c[addr],second = c[addr+1],third = c[addr+2]; //For two or three bytes
    if(first <0x80) // Positive byte, two bytes follow
      addr+=2;
    else // Negative byte, three bytes follow
      addr+=3;
    e->direction=first&0x3F; //What do bits 4 and 5 represent?
    e->destination=second;
    e->ispassable=1;
    if( (first&0x40)==0 )//if bit 6 is NOT set    
      e->ispassable=0;
    e->dirtext=get_dirtext(first);
    return addr;
}

char * get_dirtext(UCHAR dirbyte){
  UCHAR searchdir=dirbyte&0x3F;
  UCHAR di;
  long dirtable = strtol("23B5",NULL,16)-start;
  char word[16]; //Max it needs to hold is "NOT A DIRECTION"
  
  for(di=0;di<0x0E;di++){//Corresponds to index of direction commands
    UCHAR dd=c[dirtable+di];
    if(dd==searchdir)
      break;
  }
  if(di<0x0E)
    getword(word,getcommandaddress(di));
  else
    sprintf(word,"NOT A DIRECTION");
  return copytolower(word);
}

void free_location(struct location *l){
  int i;
  free(l->description);
  for(i=0;i<l->nexits;i++)
    free((l->exits)[i].dirtext);
  free(l->exits);
}

void print_short_description(struct location *locations, int i){
  printf("You are %s.\n",(locations[i]).description);
}

void print_long_description(struct location *locations, int i){
  struct location *l = locations+i;
  print_short_description(locations,i);
  for(i=0;i<l->nexits;i++){
    struct exit e=(l->exits)[i];
    printf("%s is ",e.dirtext);
    if(e.destination<NLOCATIONS){
      printf("%s",(locations[e.destination]).description);
    } else 
      printf("something not loaded.");
    printf("\n");
  }
}