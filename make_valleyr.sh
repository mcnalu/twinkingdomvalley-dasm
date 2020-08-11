#!/bin/sh

#Splits up and reassemble Twin Kingdom Valley binary
#into form it will have in BBC's memory when running.
#This relocating of bytes is performed by LVAL.

V=tkv_disk/VALLEY_
VOUT=machinecode/VALLEYR

#TKV code is loaded at $1100 ie page $11
START=11

#Page $78 is moved to $04
PAGE=78
SKIP=`echo "obase=10;ibase=16;$PAGE-$START" | bc`
dd if=$V bs=256 skip=$SKIP count=1 > $VOUT

#Pages $11-13 are moved to $05-07
PAGE=11
SKIP=`echo "obase=10;ibase=16;$PAGE-$START" | bc`
dd if=$V bs=256 skip=$SKIP count=3 >> $VOUT

#Pages $08-0A are not used so fill with zeros
dd if=/dev/zero bs=256 count=3 >> $VOUT

#Page $5E is moved to $0B
PAGE=5E
SKIP=`echo "obase=10;ibase=16;$PAGE-$START" | bc`
dd if=$V bs=256 skip=$SKIP count=1 >> $VOUT

#Pages $70-77 are moved to $0C-13
PAGE=70
SKIP=`echo "obase=10;ibase=16;$PAGE-$START" | bc`
dd if=$V bs=256 skip=$SKIP count=8 >> $VOUT

#Pages $14-5D are left in place
PAGE=14
SKIP=`echo "obase=10;ibase=16;$PAGE-$START" | bc`
N=`echo "obase=10;ibase=16;5D-14+1" | bc`
dd if=$V bs=256 skip=$SKIP count=$N >> $VOUT