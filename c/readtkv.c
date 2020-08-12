#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILESIZE 23040
#define MAXWORD 100
#define MAXLINE 1000

#define NOTHING 0
#define L14CE 1
#define L3B6C 2

//The start address of TKV code in BBC's memory 
long start;
//ALL other addresses have start subtracted from them in this code.
long lbtable;
long hbtable;
long offsettable;

//TKV code as re-ordered into BBC's memory at very start of game.
char c[FILESIZE];

long getword(char *word,long address);
long getwordaddress(long code);
long getcommandaddress(long code);
long printwords(char *line, char *s, int type);
void createbytelines(char b1, char b2, char b3);
void processdasm();
int processline(char *line, int found);

void dotests();

int main(int argc, char *argv[]) {
  fprintf(stderr,"Opening binary file...\n");
  FILE *fp = fopen("../machinecode/VALLEYR","r");

  int n;
  start = strtol("0400",NULL,16);
  lbtable = strtol("0380",NULL,16)-start;
  hbtable = strtol("0400",NULL,16)-start;  
  offsettable = strtol("3400",NULL,16)-start;

  n = fread(c,1,FILESIZE,fp);
  fclose(fp);  
  fprintf(stderr,"...closed.\n");

  if(argc>1){
    char codes[MAXLINE];
    char *ss;
    char words[MAXLINE];
    int i=0;
    ss=codes;
    for(i=1;i<argc;i++){
      sprintf(ss,"%s ",argv[i]);
      ss+=4;
    }
    printwords(words,codes,L3B6C);
    printf("%s is %s\n",codes,words);
  } else {
  //dotests();
  //createbytelines(32,108,59);
  //createbytelines(32,-50,20);
  processdasm();
  }
  return 0;
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

//Gets the address for a code <128 which corresponds to the command table
long getwordaddress(long code){
  char a[5];
  long lb=c[lbtable+code];
  long hb=c[hbtable+code];
  if(hb<0)
    hb+=256;
  if(lb<0)
    lb+=256;
  //printf("%d %d\n",lbtable+code,lb);
  sprintf(a,"%02x%02x",hb,lb);
  //printf("%s\n",a);
  return strtol(a,NULL,16)-start;
}

//Gets the address for a code >=128 which corresponds to the word table
long getcommandaddress(long code){
  long add=5*code;
  long offset = c[offsettable+code];
  if(offset<0) {
    offset+=256;
  }
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

//Turns codes terminated by $00 into words eg $98 $2D $00
//type is either L14CE or L3B6C
long printwords(char *line, char *s, int type){
  long i=0, linepos=0, l;
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
void createbytelines(char b1, char b2, char b3){
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
  } else {  //Still ore to do so return found for use in next call
    return found;
  }
}