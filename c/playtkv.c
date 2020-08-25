#include <ctype.h>
#include "libtkv.h"

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
  UCHAR destination;
  UCHAR ispassable;
  UCHAR exittype;
  UCHAR locktype;
  UCHAR isopen;
};

void load_location(struct location *l, UCHAR id);
void free_location(struct location *l);
void print_short_description(struct location *l);

int main(int argc, char *argv[]) {
  init_tkv();
  
  struct location *l = (struct location *) malloc(sizeof(struct location));
  load_location(l,0);
  print_short_description(l);
  free_location(l);
}

void load_location(struct location *l, UCHAR id){
  long addr=getlocationaddress(id);
  char desc[MAXLINE];
  int len;
  l->id=id;
  namelocation(desc,addr);
  len=strlen(desc);
  l->description=(char *) malloc(len+1);
  while(len>=0){
    (l->description)[len]=tolower(desc[len]);
    len--;
  }
}

void free_location(struct location *l){
  free(l->description);
  free(l);
}

void print_short_description(struct location *l){
  printf("You are %s.\n",l->description);
}