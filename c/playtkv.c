#include <ctype.h>
#include "libtkv.h"

#define NLOCATIONS 176
#define MAXLINE 1000

#define QUIT -1
#define EMPTY -2
#define INVALID_MOVE 0xFF
#define SUCCESS 0
#define CONTINUE 1
#define MOVED 2
#define LOOK 3
#define DRAW 4

#define NOMATCH 0xFF

#define PASSABLE 1
#define IMPASSABLE 0
#define NORMAL 0
#define NONDOOR 1
#define NONDOOR_SEETHRU 2
#define DOOR 3
#define UNLOCKED 0
#define LOCKED 1

#define UNLOCK 0
#define LOCK 1

#define NONHOSTILE 0
#define HOSTILE 1

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
  char *exittext; //This will be null for normal exits
  UCHAR destination;
  UCHAR ispassable;
  UCHAR exittype;
  UCHAR islocked;
};

struct object {
  char *description;
  UCHAR location_id;
  UCHAR size;
  UCHAR strength_required;
  UCHAR durability;
  UCHAR max_throw_damage;
  UCHAR rnd_throw_damage;
  UCHAR max_melee_damage;
  UCHAR rnd_melee_damage;
};

struct character {
  char *description;
  UCHAR location_id;
  UCHAR ishostile;
  UCHAR strength;
  UCHAR max_carried;
  UCHAR amount_carried;
};

void process_output(char *s);
int process_input();
int parse_and_process(char *line, int len);
int process(UCHAR *matches, int nmatches);
UCHAR get_match(UCHAR *word);
void report_invalid_move(struct exit *e, UCHAR dirindex, UCHAR type);

int noncombat_action(UCHAR code, UCHAR *matches, int nmatches);
int use_key(UCHAR code, UCHAR *matches, int nmatches);
void use_key_on_doors(struct exit *e);
int player_has_key(char *doortext);
int drop(UCHAR *matches, int nmatches);
int take(UCHAR *matches, int nmatches);
int inventory(UCHAR character_id);
struct object * match_object(UCHAR location_id,UCHAR *matches, int nmatches);
int count_matched_words(char *desc, UCHAR *matches, int nmatches);
int move(UCHAR dir, UCHAR dirindex);

char * create_copy_string(char *from);
char * sfirstcaps(char *s);

void load_location(struct location *l, UCHAR id);
void load_object(struct object *o, UCHAR id);
void load_character(struct character *c, UCHAR id);
long load_exit(struct exit *e, long addr);
void free_location(struct location *l);
void free_object(struct object *l);
void free_character(struct character *c);
void cleanup();

char * get_dirtext(UCHAR dirbyte);
void print_short_description(UCHAR i);
void print_long_description(UCHAR i);
void print_description(UCHAR i);
void print_objects(UCHAR i);
void print_npc_info();
char * get_nondoor_text(UCHAR firstbyte, UCHAR thirdbyte);
char * get_door_text(UCHAR firstbyte, UCHAR thirdbyte);
int print_word_or_zero(char *ss, int pos, long addr, UCHAR byte);
void you_can_see(char *text, struct location dest);

struct character * get_npc(UCHAR id);
struct location * get_location(UCHAR i);
struct character * get_player();
struct location * get_player_location();
void set_player_location_id(UCHAR i);
UCHAR get_player_location_id();
struct object * get_object(UCHAR i);

//Never access these global variables directly - use get/set methods
//Exception is during loading data and freeing location info
struct location *locations;
struct object *objects;
struct character *characters;

int main(int argc, char *argv[]) {
  init_tkv();
  int i;
  locations = (struct location *) malloc(NLOCATIONS*sizeof(struct location));
  for(i=0;i<NLOCATIONS;i++)
    load_location(locations+i,i);
  objects = (struct object *) malloc(getnumberofobjects()*sizeof(struct object));
  for(i=0;i<getnumberofobjects();i++)
    load_object(objects+i,i);
  characters = (struct character *) malloc(getnumberofcharacters()*sizeof(struct character));
  for(i=0;i<getnumberofcharacters();i++)
    load_character(characters+i,i);

  if(argc>1){
    print_description(strtol(argv[1],NULL,16));
    return 0;
  }
  
  set_player_location_id(1);
  int input=LOOK;
  do {
    if(input==MOVED || input==LOOK) {
      print_description(get_player_location_id());
    }
    if(input==MOVED || input==SUCCESS || input==CONTINUE) {//A new turn
      printf("          ---------------%02x\n",get_player()->location_id);
      print_npc_info();
    }
    process_output("?");
    input=process_input();
    //printf("input: |%d|.\n");
  } while(input!=QUIT);

  cleanup();
  return 0;
}

void process_output(char *s){
  printf("%s",s);
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
  if(i==1)
    return EMPTY;
  return parse_and_process(line,i);
}

int parse_and_process(char *line, int len){
  UCHAR matches[10];
  int i;
  int nmatches=0;
  for(i=0;i<len-1;i++){//$230D loop
    if(line[i]=='*'){//$2310
      process_output("The * commands are not implemented yet.\n");
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

int process(UCHAR *matches, int nmatches){
  //These dirbytes are in $23B5-C3
  //Correspond to: N  S  E  W  U  D  NE NW NE NW SE SW SE SW
  UCHAR dirbytes[]={0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
    0x28, 0x24, 0x28, 0x24, 0x18, 0x14, 0x18, 0x14};
  int i;
  UCHAR dir=0;
  for(i=0;i<nmatches;i++){//See $23CA
    if(matches[i]<0x0E){//A direction
      dir=dir|dirbytes[matches[i]]; //Set bits for commanded direction
    } else if(matches[i]>=0x0E && matches[i]<=0x10){
      process_output("Naughty! You are a pacifist.\n");
      return EMPTY;
    } else if(matches[i]>=0x11 && matches[i]<=0x2E){
      return noncombat_action(matches[i],matches,nmatches);
    } else {
      process_output("I don't understand.\n");
      return EMPTY;
    }

  }
  for(i=0;i<0x0E;i++)   //Find index of commanded direction
    if(dir==dirbytes[i])
      break;
  return move(dir,i);
}

//Called with code between 0x11 and 0x2E
int noncombat_action(UCHAR code, UCHAR *matches, int nmatches){
  //printf("noncombat_action: %02x.\n",code);
  switch(code){ //code is 0x11 to 0x2E
    case 0x11: return drop(matches,nmatches); //DROP
    case 0x12: case 0x13: return take(matches,nmatches); //TAKE,GET
    case 0x14: case 0x15: case 0x16: return use_key(LOCK,matches,nmatches); //LOCK,SHUT,CLOSE
    case 0x17: case 0x18: return use_key(UNLOCK,matches,nmatches); //OPEN,UNLOCK
    case 0x25: case 0x26: return QUIT; //QUIT,END
    case 0x27: case 0x28: return LOOK; //LOOK,VIEW
    case 0x29: case 0x2A: process_output("Apologies, graphics not implemented yet.\n"); return DRAW; //PICTURE,DRAW
    case 0x2B: process_output("Not implemented yet.\n"); return EMPTY; //SCORE
    case 0x2C: return inventory(0);
    case 0x2D: process_output("Not implemented yet.\n"); return EMPTY; //OPTION;
    case 0x2E: process_output("Not implemented yet.\n"); return EMPTY; //HELP;
  }
  fprintf(stderr,"This should never be printed.\n");
  return EMPTY; //Should never get here
}

int use_key(UCHAR action, UCHAR *matches, int nmatches){
  struct location *pl = get_player_location();
  int i;
  for(i=0;i<pl->nexits;i++){
    struct exit *e = pl->exits+i;
    if(e->exittype==DOOR){//The grate is classed as a DOOR
      if(action==UNLOCK && e->islocked==UNLOCKED){//$17AD
	process_output("It's already open.\n");
	return CONTINUE;
      } else if(action==LOCK && e->islocked==LOCKED){//$17C6
	process_output("It's already locked.\n");
	return CONTINUE;
      } else if(player_has_key(e->exittext)==0){//$1790
	process_output("You don't have the key.\n");
	return CONTINUE;
      } else  {//Player has the right key for the exit
	use_key_on_doors(e);
	return SUCCESS;
      }
    }
  }
  //Will only get here if no lockable exit (DOOR) is found in loop
  process_output("Nothing here locks.\n"); //$1733
  return CONTINUE;
}

//Unlocks a d
void use_key_on_doors(struct exit *e){
  struct location *dest = get_location(e->destination);
  struct exit *otherside=NULL;
  int i;
  
  //printf("otherside: %s\n",dest->description);
  for(i=0;i<dest->nexits;i++){
    otherside=dest->exits+i;
    //printf("|%s| |%s|\n",otherside->exittext, e->exittext);
    if( otherside->exittext!=NULL && strcmp(otherside->exittext, e->exittext)==0 )
      break;
  }
  if(otherside==NULL)
    fprintf(stderr,"SERIOUS ERROR: %s is one sided at location id %02x.\n",e->exittext,e->destination);
  if(e->islocked==LOCKED){ //NEED TO ALTER DESCRIPTION TEXT AND PASSABLE
    e->islocked=UNLOCKED;
    e->ispassable=PASSABLE;
    otherside->islocked=UNLOCKED;
    otherside->ispassable=PASSABLE;
    process_output("It is now opened.\n"); //Just before $178D
  } else {
    e->islocked=LOCKED;
    e->ispassable=IMPASSABLE;
    otherside->islocked=LOCKED;
    otherside->ispassable=IMPASSABLE;
    process_output("It is now locked.\n");
  }
}

int player_has_key(char *doortext){
  UCHAR id;
  for(id=0;id<getnumberofobjects();id++){
    struct object *o=get_object(id);
    if(o->location_id==0xC8){//Player has this object
      char desc[100];
      char *key;
      strcpy(desc,o->description);
      key= strstr(desc,"key");
      //printf("|%s| |%s|\n",key,doortext);
      if(key!=NULL){
	char *keytype=desc+2;//Skip "A " to point to "brass" etc
	(key-1)[0]='\0'; //Overwrite space before "key";
	if(keytype[0]=='m' //Must be the master key
	    || strstr(doortext,"metal")!=NULL //Any key matches the metal door
	    || strstr(doortext,keytype)!=NULL){ //Key type matches door eg brass, bronze etc
	  return 1;
	}
      }
    }
  }
  return 0;//Player doesn't have key
}

int inventory(UCHAR character_id){
  int id;
  int nitems=0;
  if(character_id==0)
    process_output("You have ");
  else
    process_output(" is here with ");
  //NOTE: Holdall not implemented yet.
  for(id=1;id<getnumberofobjects();id++){
    struct object *obj=get_object(id);
    if(obj->location_id==(0xC8+character_id)){
      char line[1000];
      if(nitems==0)
        process_output("the following\n");
      sprintf(line,"  %c%s\n",toupper(obj->description[0]),obj->description+1); //First letter caps
      process_output(line);
      nitems++;
    }
  }
  if(nitems==0)
    process_output("nothing.\n");
  
  return EMPTY;
}

struct object * match_object(UCHAR location_id,UCHAR *matches, int nmatches){
  UCHAR id;
  int best_count=0;
  struct object *best_object=NULL;
  for(id=0;id<getnumberofobjects();id++){
    struct object *o = get_object(id);
    if(o->location_id==location_id){
      //printf("Object is here: %s.\n",o->description);
      int count=count_matched_words(o->description,matches,nmatches);
      if(count>best_count){
	best_count=count;
	best_object=o;
      }
    }
  }
  return best_object;
}

int drop(UCHAR *matches, int nmatches){
  struct object *obj=match_object(0xC8,matches,nmatches);
  if(obj!=NULL){//Need to add in check on carried amount, see $156F
    obj->location_id=get_player_location_id();; //Location is player's position
    get_player()->amount_carried-=obj->size;
    process_output("OK.\n");
    return SUCCESS;
  } else {//To do: In game it asks "Drop what?" and gives second chance for input, see just before $15D2
    process_output("You haven't got that.\n");
    return EMPTY;
  }
}

int take(UCHAR *matches, int nmatches){
  struct object *obj=match_object(get_player_location_id(),matches,nmatches);
  if(obj!=NULL){//Need to add in check on carried amount, see $156F
    struct character *player=get_player();
    if(player->amount_carried+obj->size<player->max_carried){
      get_player()->amount_carried+=obj->size;
      obj->location_id=0xC8; //Location is player's possession
      process_output("I have it now.\n");
      return SUCCESS;
    } else {
      process_output("You can carry any more.\n");
      return CONTINUE; //Check in original if turn is indeed missed in this case.
    }
  } else {//To do: In game it asks "Take what?" and gives second chance for input, see $15D2
    process_output("I can't see any here.\n");
    return EMPTY;
  }
}

int count_matched_words(char *desc, UCHAR *matches, int nmatches){
  int i, count=0;
  for(i=0;i<nmatches;i++){
    char word[100];
    getword(word,matches[i]);
    if(strstr(desc,word)!=NULL)
      count++;
  }
  return count;
}

//Returns the new player location code if move happened.
//INVALID_MOVE if not.
int move(UCHAR dir, UCHAR dirindex){
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
    set_player_location_id(ematch->destination);
    return MOVED;
  }
}

void report_invalid_move(struct exit *e, UCHAR dirindex, UCHAR type){
  char w[100];
  char line[1000];
  getword(w, dirindex);
  //These are given in order they appear in code from $247A
  switch(type){ //type is 0-7
    case 1: sprintf(line,"Steep lag heaps block your way.\n"); break;
    case 2: sprintf(line,"The cliff is too steep.\n"); break;
    case 3: sprintf(line,"The mountains are too steep.\n"); break;
    case 4: sprintf(line,"There are bushes in your way.\n"); break;
    case 5: sprintf(line,"Dense undergrowth blocks your way.\n"); break;
    case 6: sprintf(line,"No passage leads %s.\n",w); break;
    case 7: sprintf(line,"The dunes are too steep.\n"); break;
    case 0: sprintf(line,"You can't go %s.\n",w); break;
  }
  process_output(line);
  return;
}

UCHAR get_match(UCHAR *word){
  char w[100];
  UCHAR code;
  for(code=0;code<0xFD;code++){//$FE is *** marking end of $3600 word table
    getword(w,code);
    if(strstr(w,word)==w)//Is seach term match for first part of w?
      return code;
  }
  return NOMATCH;
}

//Copys string, creating memory for to string - remember to free it!
char * create_copy_string(char *from){
  int len=strlen(from);
  char *to=(char *) malloc(len+1);
  while(len>=0){//Copy string and drop to lower case
    to[len]=from[len];
    len--;
  }
  return to;
}

char * sfirstcaps(char *s){
  s[0]=toupper(s[0]);
  return s;
}

void load_object(struct object *o, UCHAR id){
  char desc[MAXLINE];
  UCHAR  code = getbyte("2580",id);

  o->location_id=getbyte("25C0",id);
  printobjectdescription(desc, id);
  o->description=create_copy_string(desc);
  o->size = getbyte("2600",code);
  o->strength_required = getbyte("262A",code);
  o->durability = getbyte("267E",code);
  o->max_throw_damage = getbyte("26A8",code);
  o->rnd_throw_damage = getbyte("26D2",code);
  o->max_melee_damage = getbyte("26FC",code);
  o->rnd_melee_damage = getbyte("2726",code);
}

void load_character(struct character *c, UCHAR id){
  char desc[MAXLINE];
  UCHAR  classcode = getbyte("2968",id);

  c->location_id=getbyte("29B4",id);
  c->strength=getbyte("298E",id);
  printcharacterdescription(desc, classcode);
  c->description=create_copy_string(desc);
  c->ishostile=NONHOSTILE;
  if( (classcode&0x80) != 0 ){
    c->ishostile=HOSTILE;
  }
  c->amount_carried=getbyte("29DA",classcode);//Check what NPCs start carrying against this
  c->max_carried=getbyte("28E0",classcode);
  //Also need stuff in $2880 and 28A0
}

void load_location(struct location *l, UCHAR id){
  long addr=getlocationaddress(id);
  char desc[MAXLINE];
  int i;
  l->id=id;
  //Set up the description
  namelocation(desc,addr);
  l->description=create_copy_string(desc);
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
  return create_copy_string(text);
}

char * get_door_text(UCHAR firstbyte, UCHAR thirdbyte){//$2220
  char text[1000], *ret;
  UCHAR byte=thirdbyte&0x07;
  long addr=strtol("295F",NULL,16)-start; //BRASS, BRONZE etc
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
  return create_copy_string(text);
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
  char word[100];
  char *s;
  printdirectiondescription(word,dirbyte);
  s=create_copy_string(word);
  s[0]=toupper(s[0]);
  return s;
}

void print_description(UCHAR id){
  print_short_description(id);
  print_long_description(id); //Need to make this conditional
  print_objects(id);
}

void print_objects(UCHAR id){
  int i;
  for(i=0;i<getnumberofobjects();i++){
    char w[1000];
    struct object *obj=get_object(i);
    if(obj->location_id==get_player_location_id()){
      sprintf(w,"There is %s here.\n",obj->description);
      process_output(w);
    }
  }
}

void print_short_description(UCHAR id){
  struct location *l = get_location(id);
  char line[5000];
  sprintf(line,"You are %s.\n",l->description);
  process_output(line);
}

void print_long_description(UCHAR id){
  char text[5000];
  struct location *l = get_location(id);
  int i;
  for(i=0;i<l->nexits;i++){
    struct exit e=(l->exits)[i];
    struct location *dest=get_location(e.destination);
    char line[5000];
    sprintf(line,"%s ",e.dirtext);
    process_output(line);
    if(e.exittype==NORMAL) {
      you_can_see(text,*dest);
      process_output(text);
    }
    else {//door or grate or fence or something else?
      process_output("is ");
      if(e.exittype==NONDOOR || e.exittype==NONDOOR_SEETHRU){//eg fence
	process_output(e.exittext);
	if(e.exittype==NONDOOR_SEETHRU){//If dir byte bit 6 set, $2243
	  sprintf(text," through which ");//15 chars
	  you_can_see(text+15,*dest);
	  process_output(text);	  
	}
      } else {//it's a door or grate
	if(e.islocked==LOCKED)
	  process_output("a locked ");
	else
	  process_output("an open ");
	process_output(e.exittext);
	if(strstr(e.exittext,"grate")!=NULL){
	  sprintf(text," through which ");//15 chars
	  you_can_see(text+15,*dest);
	  process_output(text);
	}
      }
    }
    process_output(".\n");
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
  sprintf(text,"you can see %s",ss);
}

void print_npc_info(){
  UCHAR id;
  
  for(id=1;id<getnumberofcharacters();id++){
    struct character *npc = get_npc(id);
    if(npc->location_id==get_player()->location_id){
      char desc[100];
      sprintf(desc,"%s is here with ",npc->description);
      process_output(sfirstcaps(desc));
      inventory(id);
    }
  }
}

struct character * get_player(){
  return characters;
}

//id=0 is player
struct character * get_npc(UCHAR id){
  return characters+id;
}

struct location * get_location(UCHAR i){
  return locations+i;
}

struct object * get_object(UCHAR i){
  return objects+i;
}

struct location * get_player_location(){
  return locations+get_player_location_id();
}

UCHAR get_player_location_id(){
  return characters->location_id;
}

void set_player_location_id(UCHAR id){
  characters->location_id=id;
}

void cleanup(){
  int i;
  for(i=0;i<NLOCATIONS;i++)
    free_location(locations+i);
  for(i=0;i<getnumberofobjects();i++)
    free_object(objects+i);
  for(i=0;i<getnumberofcharacters();i++)
    free_character(characters+i);
  free(locations);
  free(objects);
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

void free_object(struct object *o){
  free(o->description);
}

void free_character(struct character *c){
  free(c->description);
}