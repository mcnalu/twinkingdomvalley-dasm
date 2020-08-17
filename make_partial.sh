#!/bin/sh

#Takes the Twin Kingdom Valley binary
#in the form it will have in BBC's memory when running
#and outputs a part of it which is hopefully all code!

V="machinecode/VALLEYR"

#Offset is where VALLEY? starts in BBC memory. Don't change!
OFFSET=0400

#Chunk of code that this script extracts. Do change!
START=3500
END=35FF

B=$V-$START-$END

#Extract relevant chunk of binary
SKIP=`echo "obase=10;ibase=16;$START-$OFFSET" | bc`
LEN=`echo "obase=10;ibase=16;$END-$START+1" | bc`
dd if=$V bs=1 skip=$SKIP count=$LEN > $B

#Write command file for BeebDis
NAME="v$START-$END"
D="$NAME-command.txt"

echo "load \$$START $B" > $D
echo "save $NAME-dasm.txt" >> $D
echo "entry \$$START" >> $D
echo "CPU 6502" >> $D
echo "symbols BBC_OS_labels.txt" >> $D
echo "hexdump" >> $D

#./BeebDis $D
