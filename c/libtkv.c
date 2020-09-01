#include "libtkv.h"

void init_tkv(){
  fprintf(stderr,"Opening binary file...\n");
  FILE *fp = fopen("../machinecode/VALLEYR","r");
  fread(ctkv,1,FILESIZE,fp);
  fclose(fp);  
  fprintf(stderr,"...closed.\n");
  
  start = strtol("0400",NULL,16);
  lbwordtable = strtol("0380",NULL,16)-start;
  hbwordtable = strtol("0400",NULL,16)-start;  
  offsettable = strtol("3400",NULL,16)-start;
  lbloctable = strtol("2A00",NULL,16)-start;
  hbloctable = strtol("2B00",NULL,16)-start; 
}

UCHAR getbyte(char *tableaddr, UCHAR y){
  long table = strtol(tableaddr,NULL,16)-start;
  return ctkv[table+y];
}

UCHAR getnumberofobjects(){
  return ctkv[strtol("27FC",NULL,16)-start]; //This is $34
}

UCHAR getnumberofcharacters(){
  return ctkv[strtol("27FE",NULL,16)-start]; //This is $34
}

void printdirectiondescription(char *word, UCHAR dirbyte){
  UCHAR searchdir=dirbyte&0x3F;
  UCHAR di;
  long dirtable = strtol("23B5",NULL,16)-start;
  for(di=0;di<0x0E;di++){//Corresponds to index of direction commands
    UCHAR dd=ctkv[dirtable+di];
    if(dd==searchdir)
      break;
  }
  if(di<0x0E)
    getwordforaddress(word,getcommandaddress(di));
  else {//Must be a compound direction
    int pos=0;
    for(di=0;di<0x06;di++){//Corresponds to index of N,S,E,W,U,D
      UCHAR dd=ctkv[dirtable+di];
      if( (dd&searchdir) !=0 )//Is the relevant bit set of dd: each one has only one bit set
	if(pos==0)
	  pos=getwordforaddress(word,getcommandaddress(di));
	else {
	  sprintf(word+pos-1," and ");
	  getwordforaddress(word+pos+4,getcommandaddress(di));
	}
    }    
  }
}

void printobjectdescription(char *line, UCHAR code){
  char addrs[NOBJWORDS][5]={"2750","277A","27A4","27CE"};
  UCHAR  lu = getbyte("2580",code);
  int  i;
  int linepos=0;
  for(i=0;i<NOBJWORDS;i++){
    UCHAR  w = getbyte(addrs[i],lu);
    if(w==0){
      line[linepos-1]='\0';
      break;
    }
    linepos+=getwordforaddress(line+linepos,getcommandaddress(w));
    if(i<NOBJWORDS-1)
      line[linepos-1]=' ';
  }
}

void printcharacterdescription(UCHAR *line, UCHAR classcode){
  char addrs[NNPCWORDS][5]={"2820","2840","2860"};
  int  i;
  int linepos=0;
  for(i=0;i<NNPCWORDS;i++){
    UCHAR w = getbyte(addrs[i],classcode);
    if(w==0){
      line[linepos-1]='\0';
      break;
    }
    linepos+=getwordforaddress(line+linepos,getcommandaddress(w));
    if(i<NNPCWORDS-1)
      line[linepos-1]=' ';
  }
}

//Gets the address for a code <128 which corresponds to the command table via 14CE
long getlocationaddress(unsigned char code){
  return getaddressfromtable(lbloctable,hbloctable,code);
}

long getaddressfromtable(long lbtable, long hbtable, unsigned char code){
  char a[5];
  unsigned char lb=ctkv[lbtable+code];
  unsigned char hb=ctkv[hbtable+code];
  //printf("%d %d\n",lbtable+code,lb);
  sprintf(a,"%02x%02x",hb,lb);
  //printf("%s\n",a);
  return strtol(a,NULL,16)-start;  
}

void namelocation(char *name, long addr){
  UCHAR b79=ctkv[addr];
  UCHAR b7A=ctkv[addr+1];
  UCHAR  nwords=b7A&0x1F;
  int i,linepos=0;
  for(i=0;i<nwords;i++){
    UCHAR l =ctkv[addr+2+i];
    if(l!=0xFF){//TBD This indicates the next word should have caps first letter
      linepos+=getwordforaddress(name+linepos,getcommandaddress(l));
      if(i<nwords-1)
	name[linepos-1]=' ';
    }
  }
  if(nwords==0){
    //printf("NOTE: This is an in code description\n");
    incodedescription(name,b79);
  }  
}

void incodedescription(char *ss, UCHAR b){
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

void incodeexits(char *ss, UCHAR b){
  //These are given in order they appear in code from $2038
  switch(b){
    case 1: sprintf(ss,"A DISUSED QUARRY"); return;
    case 2: sprintf(ss,"THE CANYON FLOOR"); return;
    case 3: sprintf(ss,"A MOUNTAIN RANGE"); return;
    case 4: sprintf(ss,"FOREST"); return;
    case 5: sprintf(ss,"FOREST"); return;
    case 6: sprintf(ss,"A PASSAGE"); return;
    default: sprintf(ss,"DESERT"); return;
  }  
}

//Gets the address for a code >=128 which corresponds to the word table
long getcommandaddress(UCHAR code){
  long add=5* (long) code; //Convert to long to avoid overflow
  long offset = (long) ctkv[offsettable+code];
 
  add += strtol("3500",NULL,16)+offset+128;
  //printf("Address is %04x and offset was %d\n",add,offset);
  return add-start;
}

//Note: last word in $3600 table has code $FE and is *** (3 asterisks)
void getword(char *w, UCHAR code){
  getwordforaddress(w,getcommandaddress(code));
}

//Writes the word stored at address into w
//Returns characters written including final \0
long getwordforaddress(char *w, long address){
  long i=0;
  do {
    w[i]=ctkv[address+i];
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