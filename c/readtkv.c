#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILESIZE 23040
#define MAXWORD 100
#define MAXLINE 1000
#define NOBJWORDS 4
#define NNPCWORDS 3

#define NOTHING 0
#define L14CE 1
#define L3B6C 2

/* To do
 * Why does location \$1D not work?
 */

//The start address of TKV code in BBC's memory 
long start;
//ALL other addresses have start subtracted from them in this code.
long lbwordtable,hbwordtable;
long lbloctable,hbloctable;

long offsettable;

//TKV code as re-ordered into BBC's memory at very start of game.
unsigned char c[FILESIZE];

long getword(char *word,long address);
long getwordaddress(unsigned char code);
long getcommandaddress(unsigned char code);
long getlocationaddress(unsigned char code);
long getaddressfromtable(long tablelb, long tablehb,unsigned char code);
long printwords(char *line, char *s, int type);
void createbytelines(unsigned char b1, unsigned char b2, unsigned char b3);
void processdasm();
int processline(char *line, int found);
unsigned char fixnegbyte(unsigned char byte);
unsigned char getbyte(char *addrs,unsigned char  y);

void printdirection(char *word, char dirbyte);
void convertcodes2words(int argc, char *argv[]);
void describelocation(int argc, char *argv[]);
void incodedescription(char *ss, unsigned char b);
void describeobject(char *scode);
void describenpcclass(char *scode);
void describenpc(char *scode);

void dotests();

int main(int argc, char *argv[]) {
  fprintf(stderr,"Opening binary file...\n");
  FILE *fp = fopen("../machinecode/VALLEYR","r");

  int n;
  start = strtol("0400",NULL,16);
  lbwordtable = strtol("0380",NULL,16)-start;
  hbwordtable = strtol("0400",NULL,16)-start;  
  offsettable = strtol("3400",NULL,16)-start;
  lbloctable = strtol("2A00",NULL,16)-start;
  hbloctable = strtol("2B00",NULL,16)-start; 

  n = fread(c,1,FILESIZE,fp);
  fclose(fp);  
  fprintf(stderr,"...closed.\n");

  if(argc>1){
    if(strstr(argv[1],"words")!=NULL){
      convertcodes2words(argc,argv);
    } else if(strstr(argv[1],"location")!=NULL){
      describelocation(argc,argv);
    } else if(strstr(argv[1],"object")!=NULL){
      describeobject(argv[2]);
    } else if(strstr(argv[1],"npcclass")!=NULL){
      describenpcclass(argv[2]);
    } else if(strstr(argv[1],"npc")!=NULL){
      describenpc(argv[2]);
    } else if(strstr(argv[1],"dotests")!=NULL){
      dotests();
    }
  } else {
  //createbytelines(32,108,59);
  //createbytelines(32,-50,20);
    processdasm();
  }
  return 0;
}

void convertcodes2words(int argc, char *argv[]){
    char codes[MAXLINE];
    char *ss;
    char words[MAXLINE];
    int i=0;
    int type=L3B6C;
    ss=codes;
    for(i=2;i<argc;i++){
      sprintf(ss,"%s ",argv[i]);
      ss+=4;
    }
    
    if(strstr(argv[1],"words1")!=NULL)
      type=L14CE;
    printwords(words,codes,type);
    printf("%s is %s\n",codes,words);
}

void printdirection(char *word, char dirbyte){
  char searchdir=dirbyte&0x3F;
  char di;
  long dirtable = strtol("23B5",NULL,16)-start;
  
  for(di=0;di<0x0E;di++){//Corresponds to index of direction commands
    char dd=c[dirtable+di];
    if(dd==searchdir)
      break;
  }
  if(di<0x0E)
    getword(word,getcommandaddress(di));
  else
    sprintf(word,"NOT A DIRECTION");
  return;
}

void describelocation(int argc, char *argv[]){
  char *arg2 = argv[2];
  long location=strtol(arg2+1,NULL,16); //Skip 1st char $ in 2nd argument
  long addr = getlocationaddress(location);
  printf("location %s is at address %04x\n",argv[2],addr+start);
  unsigned char b79=fixnegbyte(c[addr]);
  unsigned char b7A=fixnegbyte(c[addr+1]);
  unsigned char  nwords=b7A&0x1F;
  unsigned char  nexits=b7A>>5;
  int i;
  int linepos=0;
  char line[MAXLINE];  
  printf("b79=%02x, b7A=%02x, nwords=%02x, nexits=%02x\n",b79,b7A,nwords,nexits);
  for(i=0;i<nwords;i++){
    unsigned char l =fixnegbyte(c[addr+2+i]);
    linepos+=getword(line+linepos,getcommandaddress(l));
    if(i<nwords-1)
      line[linepos-1]=' ';
  }
  printf("Exits: ");
  long j=addr+2+nwords;
  for(i=0;i<nexits;i++){
    unsigned char  first = c[j];
    if(first>=0 && first <0x80){ // Positive byte
      unsigned char  dir=fixnegbyte(first)&0x3F;
      unsigned char  dest=fixnegbyte(c[j+1]);
      char word[MAXWORD];
      printdirection(word,dir);
      printf(" %s(%02x)>%02x",word,dir,dest);
      j+=2;
    } else { //If first byte is negative then three follow, see $0DE6 but dunno why
      printf(" SPECIAL:%02x,%02x,%02x",first,c[j+1],c[j+2]);
      j+=3;
    }
  }
  if(nwords==0){
    printf("\nNOTE: This is an in code description\n");
    incodedescription(line,b79);
  } else
    printf("\n");
  printf("|%s|\n",line);
}

void incodedescription(char *ss, unsigned char b){
  //These are given in order they appear in code from $2122
  switch(b){
    case 6: sprintf(ss,"IN A SLOPING MAZE"); return;
    case 2: sprintf(ss,"ON THE CANYON FLOOR"); return;
    case 7: sprintf(ss,"IN AN ARID DESERT"); return;
    case 3: sprintf(ss,"WANDERING THROUGH HIGH MOUNTAINS, PATHS LEAD"); return;
    case 4: sprintf(ss,"WANDERING THROUGH THE WOODS, PATHS LEAD"); return;
    default: sprintf(ss,"WANDERING THROUGH DENSE FOREST, PATHS LEAD"); return;
  }
}

void describeobject(char *scode){
  unsigned char code=(unsigned char) strtol(scode+1,NULL,16);
  unsigned char  loc = getbyte("25C0",code);
  unsigned char  lu = getbyte("2580",code);
  unsigned char  requiredstr = getbyte("262A",code);
  unsigned char  weapondur = getbyte("267E",code);
  unsigned char  meldamagernd = getbyte("2726",code);
  unsigned char  meldamagemax = getbyte("26FC",code);
  unsigned char  thrdamagernd = getbyte("26D2",code);
  unsigned char  thrdamagemax = getbyte("26A8",code);
  unsigned char  size = getbyte("2600",lu);
  char addrs[NOBJWORDS][5]={"2750","277A","27A4","27CE"};
  int  i;
  int linepos=0;
  char line[MAXLINE];
  
  //printf("%s\n",scode);
  if(code<0 || code>=42){
    printf("There are only 42 objects so index must be 0-41 or $0-29.\n");
    return;
  }
  
  for(i=0;i<NOBJWORDS;i++){
    unsigned char  w = getbyte(addrs[i],lu);
    if(w==0){
      line[linepos-1]='\0';
      break;
    }
    linepos+=getword(line+linepos,getcommandaddress(w));
    if(i<NOBJWORDS-1)
      line[linepos-1]=' ';
  }
  printf("Object %02x\nLookup code %02x\nLocation %02x\nSize %02x\n",code,lu,loc,size);
  printf("Required strength %02x\n",requiredstr);
  printf("Weapon durability %02x\n",weapondur);
  printf("Throw damage: max %02x, rnd sub %02x\n",thrdamagemax,thrdamagernd);  
  printf("Melee damage: max %02x, rnd sub %02x\n",meldamagemax,meldamagernd);  
  printf("|%s|\n",line);
}

void describenpc(char *scode){
  unsigned char code= (unsigned char) strtol(scode+1,NULL,16);
  unsigned char class= (unsigned char) getbyte("2968",code);
  unsigned char health= (unsigned char) getbyte("298E",code);
  unsigned char location= (unsigned char) getbyte("29B4",code);
  char a[4];
  sprintf(a,"$%02x",class&0x7F);
  printf("Class code is %02x\n",class);
  printf("Health is %02x\n",health);
  printf("Location is %02x\n",location);  
  printf("NPC class %s is ",a);
  if((class&0x80)==0)
    printf("not ");
  printf("hostile to player\n");
  describenpcclass(a);
}

void describenpcclass(char *scode){
  unsigned char code= (unsigned char) strtol(scode+1,NULL,16);
  //long lu = getbyte("28A0",code)&0x1F; This lookup is used in $1BEA but dunno why
  char addrs[NNPCWORDS][5]={"2820","2840","2860"};
  int  i;
  int linepos=0;
  char line[MAXLINE];
  
  //printf("%s\n",scode);
  if(code<0 || code>=15){
    printf("There are only 15 things in this table so index must be $00 to $0E.\n");
    return;
  }
  
  for(i=0;i<NNPCWORDS;i++){
    unsigned char w = getbyte(addrs[i],code);
    if(w==0){
      line[linepos-1]='\0';
      break;
    }
    linepos+=getword(line+linepos,getcommandaddress(w));
    if(i<NNPCWORDS-1)
      line[linepos-1]=' ';
  }
  printf("NPC %02x\n|%s|\n",code,line);
}


unsigned char getbyte(char *tableaddr, unsigned char y){
  long table = strtol(tableaddr,NULL,16)-start;
  return fixnegbyte(c[table+y]);
}

//Performs some tests
void dotests(){
  char word[MAXWORD]; 
  char line[MAXLINE];  
  char *testaddr="057C";
  long l = strtol(testaddr,NULL,16)-start;
  printf("Should be 53 or 83 or [S]: %x or %d or [%c]\n",c[l],c[l],c[l]);  
  getword(word,l);
  printf("Word at %s |%s|\n",testaddr,word);
  printf("Address for code &98 is %04x\n",start+getwordaddress(strtol("98",NULL,16)));
  printf("Address for code &1A is %04x\n",start+getcommandaddress(strtol("1A",NULL,16)));
  char *teststr="$2A,$29,$94,$00";
  printwords(line,teststr,L14CE);   //This is the right routine for this string
  //printwords(line,teststr,L3B6C); //This is the wrong routine for this string
  printf("|%s| gives |%s|\n",teststr,line);
}

//Gets the address for a code <128 which corresponds to the command table via 14CE
long getwordaddress(unsigned char code){
  return getaddressfromtable(lbwordtable,hbwordtable,code);
}

//Gets the address for a code <128 which corresponds to the command table via 14CE
long getlocationaddress(unsigned char code){
  return getaddressfromtable(lbloctable,hbloctable,code);
}

long getaddressfromtable(long lbtable, long hbtable, unsigned char code){
  char a[5];
  unsigned char lb=c[lbtable+code];
  unsigned char hb=c[hbtable+code];
  hb=fixnegbyte(hb);
  lb=fixnegbyte(lb);
  //printf("%d %d\n",lbtable+code,lb);
  sprintf(a,"%02x%02x",hb,lb);
  //printf("%s\n",a);
  return strtol(a,NULL,16)-start;  
}

unsigned char fixnegbyte(unsigned char byte){
/* Not needed now I've switched to unsigned chars 
 * if(byte<0)
    return byte+256;
  else */
    return byte;
}

//Gets the address for a code >=128 which corresponds to the word table
long getcommandaddress(unsigned char code){
  long add=5* (long) code; //Convert to long to avoid overflow
  long offset = (long) fixnegbyte(c[offsettable+code]);
 
  add += strtol("3500",NULL,16)+offset+128;
  //printf("Address is %04x and offset was %d\n",add,offset);
  return add-start;
}

//Writes the word stored at address into w
long getword(char *w, long address){
  long i=0;
  do {
    w[i]=c[address+i];
    if(w[i]<0){
      w[i]+=128;
      break;
    }
    //printf("Char: %x or %d or [%c]\n",word[i],word[i],word[i]);  
    i++;
  } while(i<MAXWORD-1);
  w[++i]='\0';
  return i+1;
}

/*Turns codes terminated by $00 into words eg $98 $2D $00
  type is either L14CE or L3B6C
  NOTE: There's a bug in that a code of $00 meaning NORTH
  will treated as the terminator and cause a '\0' to be
  inserted. */
long printwords(char *line, char *s, int type){
  long i=0, linepos=0;
  unsigned char l;
  char b[3];
  do {
    if(s[i]=='$'){
      b[0]=s[++i];
      b[1]=s[++i];
      b[2]='\0';
      l = strtol(b,NULL,16);
      //printf("l=%d\n",l);
      if(l==0){
	line[linepos]='\0';
	break;
      }
      if(linepos>0){
	line[linepos-1]=' ';
      }
      if(type==L3B6C || l<128){
	linepos+=getword(line+linepos,getcommandaddress(l));	
      } else if(l<256){
	linepos+=getword(line+linepos,getwordaddress(l));		
      }
      //printf("%d |%s|\n",linepos,line);
    }
    i++;
  } while(s[i]!='\0');
  return l;
}

//Finds all lines with JSR $XXXX and outputs byte lines for use with BeebDis
//b1=32 and b2 is the low byte and b3 the high byte of address in two complement
void createbytelines(unsigned char b1, unsigned char b2, unsigned char b3){
  long i=0,j;
  do{
    if(c[i]==b1 && c[i+1]==b2 && c[i+2]==b3){//JSR 14CE
      j=0;
      i+=3;
      printf("byte $%04x ",start+i);
      do {
         j++;
      } while(c[i+j]!=0);
      printf("$%x\nentry pc\n",j+1);
      i=i+j;
    }
    i++;
  } while(i<FILESIZE-2);
}

//Given disassembled output from BeebDis using output from createbytelines
//via stdin send the file to stdout with byte lines after each JSR $14CE line
//converted to words. All other lines should be unchanged.
void processdasm(){
  int d, j, dj=0, found=0;
  char line[MAXLINE];
  for(j=0; dj<MAXLINE-1 && (d=getchar())!=EOF; ++j){
    if(d=='\n'){
      line[dj]='\0';
      if(found==0){
	printf("%s\n",line);
      }
      found=processline(line,found);
      dj=0;
    } else {
      line[dj++]=d;
    }
  }
}

//Process an individual line for processdasm
int processline(char *line, int found){
  char *jsr1="JSR     L14CE";
  char *jsr2="JSR     L3B6C";  
  char nuline[MAXLINE];
  int ret=-1;
  if(found==0){
    if(strstr(line,jsr1)!=NULL)
      return L14CE;
    else if (strstr(line,jsr2)!=NULL)
      return L3B6C;
    else
      return NOTHING;
  } else {
    char *ss=strchr(line,'$');
    if(ss!=NULL){
      ret=printwords(nuline,ss,found);
      printf("        %s\n",nuline);
    }
  }
  if(ret==0){//$00 was found on this line so end the conversion
    return NOTHING;
  } else {  //Still more to do so return found for use in next call
    return found;
  }
}