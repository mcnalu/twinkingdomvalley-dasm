#include "libtkv.h"

/* To do
 */

long getwordforaddress(char *word,long address);
long getwordaddress(UCHAR code);
long printwords(char *line, char *s, int type);
void createbytelines(UCHAR b1, UCHAR b2, UCHAR b3);
void processdasm();
int processline(char *line, int found);

void printdirection(char *word, char dirbyte);
void convertcodes2words(int argc, char *argv[]);
void describecommand(char *scode);
void describelocation(int argc, char *argv[]);
void describeobject(char *scode);
void describenpcclass(char *scode);
void describenpc(char *scode);

void dotests();

int main(int argc, char *argv[]) {
  init_tkv();
  if(argc>1){
    if(strstr(argv[1],"words")!=NULL){
      convertcodes2words(argc,argv);
    } else if(strstr(argv[1],"command")!=NULL){
      describecommand(argv[2]);
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
    char dd=ctkv[dirtable+di];
    if(dd==searchdir)
      break;
  }
  if( (dirbyte&0x40)==0 ){//if bit 6 is NOT set
    sprintf(word,"IMPASSABLE:");
    word=word+11;
  }
  if(di<0x0E)
    getwordforaddress(word,getcommandaddress(di));
  else
    sprintf(word,"NOT A DIRECTION");
  return;
}

long describeexit(long addr){
    UCHAR  first = ctkv[addr],second = ctkv[addr+1],third = ctkv[addr+2]; //For two or three bytes
    if(first <0x80) // Positive byte
      addr+=2;
    else // Negative byte
      addr+=3;
    UCHAR  dir=first&0x3F;
    UCHAR  dest=second;
    char direction[MAXWORD],destination[MAXWORD];
    printdirection(direction,first);
    namelocation(destination,getlocationaddress(dest));
    printf("%s>%s: %02x>%02x",direction,destination,first,dest);
    if(first>=0x80) { 
      printf(",%02x",third);
    }
    printf("\n");
    
    return addr;
}

void describelocation(int argc, char *argv[]){
  char *arg2 = argv[2];
  long location=strtol(arg2+1,NULL,16); //Skip 1st char $ in 2nd argument
  long addr = getlocationaddress(location);
  printf("location %s is at address %04x\n",argv[2],addr+start);
  UCHAR b79=ctkv[addr];
  UCHAR b7A=ctkv[addr+1];
  UCHAR  nwords=b7A&0x1F;
  UCHAR  nexits=b7A>>5;
  int i;
  char line[MAXLINE];  
  printf("b79=%02x, b7A=%02x, nwords=%02x, nexits=%02x\n",b79,b7A,nwords,nexits);
  namelocation(line,addr);
  printf("|%s|\n",line);
  printf("Exits:\n");
  long j=addr+2+nwords;
  for(i=0;i<nexits;i++){
    j=describeexit(j);
  }
}

void describecommand(char *scode){
  UCHAR id=(UCHAR) strtol(scode+1,NULL,16);
  char word[100];
  getword(word,id);
  printf("Command %02x is %s\n",id,word);
  if(id>=0x11 && id<=0x2E){//non-direction and non-combat
    printf("Address of this command is %02x%02x.\n",getbyte("1ECF",id),getbyte("1EB1",id));
  }
}

void describeobject(char *scode){
  UCHAR id=(UCHAR) strtol(scode+1,NULL,16);
  UCHAR  loc = getbyte("25C0",id);
  UCHAR  code = getbyte("2580",id);
  UCHAR  requiredstr = getbyte("262A",code);
  UCHAR  weapondur = getbyte("267E",code);
  UCHAR  meldamagernd = getbyte("2726",code);
  UCHAR  meldamagemax = getbyte("26FC",code);
  UCHAR  thrdamagernd = getbyte("26D2",code);
  UCHAR  thrdamagemax = getbyte("26A8",code);
  UCHAR  size = getbyte("2600",code);
  char line[MAXLINE];
  
  //printf("%s\n",scode);
  if(id<0 || id>=42){
    printf("There are only 42 objects so index must be 0-41 or $0-29.\n");
    return;
  }
  
  printobjectdescription(line,id);
  
  printf("Object %02x\nLookup code %02x\nLocation %02x\nSize %02x\n",id,code,loc,size);
  printf("Required strength %02x\n",requiredstr);
  printf("Weapon durability %02x\n",weapondur);
  printf("Throw damage: max %02x, rnd sub %02x\n",thrdamagemax,thrdamagernd);  
  printf("Melee damage: max %02x, rnd sub %02x\n",meldamagemax,meldamagernd);  
  printf("|%s|\n",line);
}

void describenpc(char *scode){
  UCHAR code= (UCHAR) strtol(scode+1,NULL,16);
  UCHAR class= (UCHAR) getbyte("2968",code);
  UCHAR health= (UCHAR) getbyte("298E",code);
  UCHAR location= (UCHAR) getbyte("29B4",code);
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
  UCHAR classcode= (UCHAR) strtol(scode+1,NULL,16);
  //long lu = getbyte("28A0",code)&0x1F; This lookup is used in $1BEA but dunno why
  char line[MAXLINE];
  
  //printf("%s\n",scode);
  if(classcode<0 || classcode>=15){
    printf("There are only 15 things in this table so index must be $00 to $0E.\n");
    return;
  }
  printcharacterdescription(line,classcode);
  printf("NPC %02x\n|%s|\n",classcode,line);
}

//Performs some tests
void dotests(){
  char word[MAXWORD]; 
  char line[MAXLINE];  
  char *testaddr="057C";
  long l = strtol(testaddr,NULL,16)-start;
  printf("Should be 53 or 83 or [S]: %x or %d or [%c]\n",ctkv[l],ctkv[l],ctkv[l]);  
  getwordforaddress(word,l);
  printf("Word at %s |%s|\n",testaddr,word);
  printf("Address for code &98 is %04x\n",start+getwordaddress(strtol("98",NULL,16)));
  printf("Address for code &1A is %04x\n",start+getcommandaddress(strtol("1A",NULL,16)));
  char *teststr="$2A,$29,$94,$00";
  printwords(line,teststr,L14CE);   //This is the right routine for this string
  //printwords(line,teststr,L3B6C); //This is the wrong routine for this string
  printf("|%s| gives |%s|\n",teststr,line);
}

//Gets the address for a code <128 which corresponds to the command table via 14CE
long getwordaddress(UCHAR code){
  return getaddressfromtable(lbwordtable,hbwordtable,code);
}

/*Turns codes terminated by $00 into words eg $98 $2D $00
  type is either L14CE or L3B6C
  NOTE: There's a bug in that a code of $00 meaning NORTH
  will treated as the terminator and cause a '\0' to be
  inserted. */
long printwords(char *line, char *s, int type){
  long i=0, linepos=0;
  UCHAR l;
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
	linepos+=getwordforaddress(line+linepos,getcommandaddress(l));	
      } else if(l<256){
	linepos+=getwordforaddress(line+linepos,getwordaddress(l));		
      }
      //printf("%d |%s|\n",linepos,line);
    }
    i++;
  } while(s[i]!='\0');
  return l;
}

//Finds all lines with JSR $XXXX and outputs byte lines for use with BeebDis
//b1=32 and b2 is the low byte and b3 the high byte of address in two complement
void createbytelines(UCHAR b1, UCHAR b2, UCHAR b3){
  long i=0,j;
  do{
    if(ctkv[i]==b1 && ctkv[i+1]==b2 && ctkv[i+2]==b3){//JSR 14CE
      j=0;
      i+=3;
      printf("byte $%04x ",start+i);
      do {
         j++;
      } while(ctkv[i+j]!=0);
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