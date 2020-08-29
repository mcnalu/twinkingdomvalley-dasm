#include <ctype.h>
#include "libtkv.h"

#define NLOCATIONS 176
#define MAXLINE 1000

#define QUIT -1
#define EMPTY -2
#define INVALID_MOVE 0xFF

#define NOMATCH 0xFF

#define PASSABLE 1
#define IMPASSABLE 0
#define NORMAL 0
#define NONDOOR 1
#define NONDOOR_SEETHRU 2
#define DOOR 3
#define UNLOCKED 0
#define LOCKED 1

struct location {
  UCHAR id;
  char *description;
  UCHAR nexits;
  UCHAR locationtype;
  struct exit *exits;
  UCHAR isdark;
  UCHAR islongdesc;
  UCHAR isvisited;
};

struct exit {
  UCHAR direction; //first&0x3F - bits 6 and 7 info in ispassable etc
  char *dirtext;
  char *exittext;
  UCHAR destination;
  UCHAR ispassable;
  UCHAR exittype;
  UCHAR islocked;
};

int process_input();
UCHAR parse_and_process(char *line, int len);
UCHAR process(UCHAR *matches, int nmatches);
UCHAR move(UCHAR dir, UCHAR dirindex);
UCHAR get_match(UCHAR *word);
UCHAR report_invalid_move(struct exit *e, UCHAR dirindex, UCHAR type);

char * copytolower(char *from);
char * stolower(char *s);

void load_location(struct location *l, UCHAR id);
long load_exit(struct exit *e, long addr);
char * get_dirtext(UCHAR dirbyte);
void free_location(struct location *l);
void print_short_description(UCHAR i);
void print_long_description(UCHAR i);
char * get_nondoor_text(UCHAR firstbyte, UCHAR thirdbyte);
char * get_door_text(UCHAR firstbyte, UCHAR thirdbyte);
int print_word_or_zero(char *ss, int pos, long addr, UCHAR byte);
void you_can_see(char *text, struct location dest);

struct location * get_location(UCHAR i);
struct location * get_player_location();
void set_player_location_code(UCHAR i);
UCHAR get_player_location_code();

//Never access these global variables directly - use get/set methods
//Exception is during loading data and freeing location info
struct location *locations;
UCHAR player_location_code;

int main(int argc, char *argv[]) {
  init_tkv();
  int i;
  locations = (struct location *) malloc(NLOCATIONS*sizeof(struct location));
  for(i=0;i<NLOCATIONS;i++){
    load_location(locations+i,i);
    //print_short_description(locations,i);
  }

  if(argc>1){
    print_long_description(strtol(argv[1],NULL,16));
    return 0;
  }
  
  set_player_location_code(1);
  int input=1;
  do {
    if(input!=EMPTY && input!=INVALID_MOVE)
      print_long_description(get_player_location_code());
    printf("?");
    input=process_input();
  } while(input!=QUIT);

  for(i=0;i<NLOCATIONS;i++)
    free_location(locations+i);
  free(locations);
}

int process_input(){
  char line[MAXLINE];
  int c,i=0;

  do {
    c=getchar();
    line[i++]=c;
  } while( i<MAXLINE && c!=EOF && c!='\n' );
  line[i-1]='\0';
  
  //printf("|%s|\n",line);
  if(strstr(line,"quit")!=NULL)
    return QUIT;
  else if(i==1)
    return EMPTY;
  stolower(line);//Drop to lower case
  return parse_and_process(line,i);
}

UCHAR parse_and_process(char *line, int len){
  UCHAR matches[10];
  int i;
  int nmatches=0;
  for(i=0;i<len-1;i++){//$230D loop
    if(line[i]=='*'){//$2310
      printf("The * commands are not implemented yet.\n");
      return EMPTY;
    }
    char word[100];
    int j=0;
    while(line[i]>='a' && line[i]<='z' && i<len-1){//$231E
      word[j++]=line[i++];
    }
    word[j]='\0';
    matches[nmatches]=get_match(word);
    //printf("Match result for %s: %02x\n",word,matches[nmatches]);
    if(matches[nmatches]!=NOMATCH && nmatches<10)
      nmatches++;
  }
  return process(matches, nmatches);
}

UCHAR process(UCHAR *matches, int nmatches){
  //These dirbytes are in $23B5-C3
  //Correspond to: N  S  E  W  U  D  NE NW NE NW SE SW SE SW
  UCHAR dirbytes[]={0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
    0x28, 0x24, 0x28, 0x24, 0x18, 0x14, 0x18, 0x14};
  int i;
  UCHAR dir=0;
  for(i=0;i<nmatches;i++){//See $23CA
    if(matches[i]<0x0E){//A direction
      dir=dir|dirbytes[matches[i]]; //Set bits for commanded direction
    }
  }
  for(i=0;i<0x0E;i++)   //Find index of commanded direction
    if(dir==dirbytes[i])
      break;
  return move(dir,i);
}

//Returns the new player location code if move happened.
//INVALID_MOVE if not.
UCHAR move(UCHAR dir, UCHAR dirindex){
  int i;
  if(dir==0)
    return INVALID_MOVE;
  struct location *l = get_player_location();
  struct exit *ematch=NULL;
  for(i=0;i<l->nexits;i++){//$23E3 look for exact match
    struct exit *e = (l->exits)+i;
    if(e->direction==dir){
      ematch=e;
      break;
    } 
  }
  if(ematch==NULL){//Failed to find exact match
    for(i=0;i<l->nexits;i++){//$23FF look for partial match
      struct exit *e = (l->exits)+i;
      if( ( e->direction|dir) == e->direction ){ //eg E will match NE
	ematch=e;
	break;
      } 
    }    
  }
  UCHAR type=get_player_location()->locationtype;
  if(ematch==NULL){
    if( (dir&1) !=0 )//User asked to go down, $2418
      type=0;
    report_invalid_move(ematch,dirindex,type);
    return INVALID_MOVE;
  } else {
    if(ematch->ispassable==IMPASSABLE){
      report_invalid_move(ematch,dirindex,0);
      return INVALID_MOVE;
    } 
    set_player_location_code(ematch->destination);
    return ematch->destination;
  }
}

UCHAR report_invalid_move(struct exit *e, UCHAR dirindex, UCHAR type){
  char w[100];
  getword(w, dirindex);
  //These are given in order they appear in code from $247A
  switch(type){ //type is 0-7
    case 1: printf("Steep lag heaps block your way.\n"); break;
    case 2: printf("The cliff is too steep.\n"); break;
    case 3: printf("The mountains are too steep.\n"); break;
    case 4: printf("There are bushes in your way.\n"); break;
    case 5: printf("Dense undergrowth blocks your way.\n"); break;
    case 6: printf("No passage leads %s.\n",stolower(w)); break;
    case 7: printf("The dunes are too steep.\n"); break;
    case 0: printf("You can't go %s.\n",stolower(w)); break;
  }
  return type;
}

UCHAR get_match(UCHAR *word){
  char w[100];
  UCHAR code;
  for(code=0;code<0xFD;code++){//$FE is *** marking end of $3600 word table
    getword(w,code);
    stolower(w);
    if(strstr(w,word)==w)//Is seach term match for first part of w?
      return code;
  }
  return NOMATCH;
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

char * stolower(char *s){
  int len=strlen(s);
  while(len>=0){//Copy string and drop to lower case
    s[len]=tolower(s[len]);
    len--;
  }
  return s;
}

void load_location(struct location *l, UCHAR id){
  long addr=getlocationaddress(id);
  char desc[MAXLINE];
  int i;
  l->id=id;
  //Set up the description
  namelocation(desc,addr);
  l->description=copytolower(desc);
  UCHAR b79=ctkv[addr];
  l->locationtype=b79&0x07; //Used for describing exits to this place, see $2038
  //Work out number of exits
  UCHAR b7A=ctkv[addr+1];
  UCHAR  nwords=b7A&0x1F;
  l->nexits=b7A>>5;
  l->exits = (struct exit *) malloc(l->nexits*sizeof(struct exit));
  addr+=2+nwords;  //Address for direction/destination bytes
  for(i=0;i<l->nexits;i++){
    addr=load_exit((l->exits)+i,addr);
  }
}

long load_exit(struct exit *e, long addr){
    UCHAR  first = ctkv[addr],second = ctkv[addr+1],third = ctkv[addr+2]; //For two or three bytes
    if(first <0x80) // Positive byte, two bytes follow
      addr+=2;
    else // Negative byte, three bytes follow
      addr+=3;
    e->direction=first&0x3F; //What do bits 4 and 5 represent?
    e->destination=second;
    e->ispassable=PASSABLE;
    e->exittype=NORMAL;
    e->exittext=NULL;
    //See $21EE for logic of following
    if( (first&0x80)!=0 ) {//Triplet of bytes ie fence, door, grate etc
      if( (third&0x80)==0 ){//Third byte bit 7 not set, not a door or grate
	if((third&0x40)!=0) //Bit six set, see $226F
	  e->exittype=NONDOOR_SEETHRU;
	else
          e->exittype=NONDOOR; //$2255
	e->exittext=get_nondoor_text(first,third);
      } else {//Third byte set so set lockable to type of key/door
	e->exittype=DOOR;
	if( (first&0x40) == 0 ) //See just before $2217
	  e->islocked=LOCKED;
	else
	  e->islocked=UNLOCKED;
	e->exittext=get_door_text(first,third);
      }      
    }
    if( (first&0x40)==0 )//if bit 6 is NOT set    
      e->ispassable=IMPASSABLE;
    e->dirtext=get_dirtext(first);
    return addr;
}

char * get_nondoor_text(UCHAR firstbyte, UCHAR thirdbyte){//$2255 Load text as per $2255
  char text[1000], *ret;
  UCHAR byte=thirdbyte&0x07;
  long addr=strtol("295F",NULL,16)-start;
  int pos=2;
  sprintf(text,"a ");
  if(byte!=0)
    pos=print_word_or_zero(text, pos, addr, byte);

  byte=(thirdbyte>>3)&0x0F;//See $2255
  if( (firstbyte&0x40)!=0 )//If dir byte bit 6 set
    byte=byte|0x20;        //set bit 5  
  addr=strtol("2900",NULL,16)-start;  //$2275
  pos=print_word_or_zero(text, pos, addr, byte);
  addr=strtol("2930",NULL,16)-start;
  pos=print_word_or_zero(text, pos, addr, byte);
  //The "through which" bit is handled in-game as dest structs not all loaded here
  
  text[pos-1]='\0';//Last char will be space, overwrite.
  return copytolower(text);
}

char * get_door_text(UCHAR firstbyte, UCHAR thirdbyte){//$2220
  char text[1000], *ret;
  UCHAR byte=thirdbyte&0x07;
  long addr=strtol("295F",NULL,16)-start;
  int pos=0;
  if(byte!=0)
    pos=print_word_or_zero(text, pos, addr, byte);

  byte=(thirdbyte>>3)&0x0F;//See $2255
  byte=byte|0x10;        //set bit 4  
  addr=strtol("2900",NULL,16)-start; //$2275
  pos=print_word_or_zero(text, pos, addr, byte);
  addr=strtol("2930",NULL,16)-start;
  pos=print_word_or_zero(text, pos, addr, byte);
  //The "through which" bit is handled in-game as dest structs not all loaded here
  
  text[pos-1]='\0';//Last char will be space, overwrite.
  return copytolower(text);
}

/* If byte!=0, print word specified by address addr+byte into pos in ss.
   Return value is pos of final space.
   Print nothing if byte==0: pos is returned unchanged.
   See $2275 */
int print_word_or_zero(char *ss, int pos, long addr, UCHAR byte){
  byte=ctkv[addr+byte];
  if(byte!=0){//if zero print nothing, see $1FD0
    addr=getcommandaddress(byte);
    pos+=getwordforaddress(ss+pos,addr);
    ss[pos-1]=' ';
  }
  return pos;
}

char * get_dirtext(UCHAR dirbyte){
  UCHAR searchdir=dirbyte&0x3F;
  UCHAR di;
  long dirtable = strtol("23B5",NULL,16)-start;
  char word[16]; //Max it needs to hold is "NOT A DIRECTION"
  char *s;
  for(di=0;di<0x0E;di++){//Corresponds to index of direction commands
    UCHAR dd=ctkv[dirtable+di];
    if(dd==searchdir)
      break;
  }
  if(di<0x0E)
    getwordforaddress(word,getcommandaddress(di));
  else
    sprintf(word,"NOT A DIRECTION");
  s=copytolower(word);
  s[0]=toupper(s[0]);
  return s;
}

void print_short_description(UCHAR i){
  struct location *l = get_location(i);
  printf("You are %s.\n",l->description);
}

void print_long_description(UCHAR i){
  char text[1000];
  struct location *l = get_location(i);
  print_short_description(i);
  for(i=0;i<l->nexits;i++){
    struct exit e=(l->exits)[i];
    struct location *dest=get_location(e.destination);
    printf("%s ",e.dirtext);
    if(e.exittype==NORMAL) {
      you_can_see(text,*dest);
      printf("%s",text);
    }
    else {//door or grate or fence or something else?
      printf("is ");
      if(e.exittype==NONDOOR || e.exittype==NONDOOR_SEETHRU){//eg fence
	printf("%s",e.exittext);
	if(e.exittype==NONDOOR_SEETHRU){//If dir byte bit 6 set, $2243
	  sprintf(text," through which ");//15 chars
	  you_can_see(text+15,*dest);
	  printf("%s",text);	  
	}
      } else {//it's a door or grate
	if(e.islocked=LOCKED)
	  printf("a locked ");
	else
	  printf("an open ");
	printf("%s",e.exittext);
	if(strstr(e.exittext,"grate")!=NULL){
	  sprintf(text," through which ");//15 chars
	  you_can_see(text+15,*dest);
	  printf("%s",text);
	}
      }
    }
    printf(".\n");
  }
}

void you_can_see(char *text, struct location dest){
  char *ss;
  char s[20];
  ss=s;
  if(dest.locationtype==0)
    ss=strchr(dest.description,' ')+1; //Skip first word
  else
    incodeexits(ss,dest.locationtype);
  sprintf(text,"you can see %s",stolower(ss));
}

void free_location(struct location *l){
  int i;
  free(l->description);
  for(i=0;i<l->nexits;i++){
    free((l->exits)[i].dirtext);
    if((l->exits)[i].exittext != NULL )
      free((l->exits)[i].exittext);
  }
  free(l->exits);
}

struct location * get_location(UCHAR i){
  return locations+i;
}

struct location * get_player_location(){
  return locations+player_location_code;
}

UCHAR get_player_location_code(){
  return player_location_code;
}

void set_player_location_code(UCHAR i){
  player_location_code=i;
}