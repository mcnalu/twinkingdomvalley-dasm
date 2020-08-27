#include "libtkv.h"

void init_tkv(){
  fprintf(stderr,"Opening binary file...\n");
  FILE *fp = fopen("../machinecode/VALLEYR","r");
  fread(c,1,FILESIZE,fp);
  fclose(fp);  
  fprintf(stderr,"...closed.\n");
  
  start = strtol("0400",NULL,16);
  lbwordtable = strtol("0380",NULL,16)-start;
  hbwordtable = strtol("0400",NULL,16)-start;  
  offsettable = strtol("3400",NULL,16)-start;
  lbloctable = strtol("2A00",NULL,16)-start;
  hbloctable = strtol("2B00",NULL,16)-start; 
}

//Gets the address for a code <128 which corresponds to the command table via 14CE
long getlocationaddress(unsigned char code){
  return getaddressfromtable(lbloctable,hbloctable,code);
}

long getaddressfromtable(long lbtable, long hbtable, unsigned char code){
  char a[5];
  unsigned char lb=c[lbtable+code];
  unsigned char hb=c[hbtable+code];
  //printf("%d %d\n",lbtable+code,lb);
  sprintf(a,"%02x%02x",hb,lb);
  //printf("%s\n",a);
  return strtol(a,NULL,16)-start;  
}

void namelocation(char *name, long addr){
  UCHAR b79=c[addr];
  UCHAR b7A=c[addr+1];
  UCHAR  nwords=b7A&0x1F;
  int i,linepos=0;
  for(i=0;i<nwords;i++){
    UCHAR l =c[addr+2+i];
    linepos+=getword(name+linepos,getcommandaddress(l));
    if(i<nwords-1)
      name[linepos-1]=' ';
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
  long offset = (long) c[offsettable+code];
 
  add += strtol("3500",NULL,16)+offset+128;
  //printf("Address is %04x and offset was %d\n",add,offset);
  return add-start;
}

//Writes the word stored at address into w
//Returns characters written including final \0
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