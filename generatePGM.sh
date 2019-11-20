#! /bin/bash

# This picks a line from the CSV export and attempts to convert the binary to decimals
# to generate a PGM of the given size
# ./generatePGM.sh <CSV file> <Frame> <Width> <Height> 

BYTES=`grep "^$2" $1 | cut -d, -f4 | sed 's/:/ /g'`
DECIMALS=""
for BYTE in $BYTES
do 
  DECIMALS=`printf "$DECIMALS % 3d" 0x$BYTE`
done
echo "P2"
echo "$3 $4"
echo "255"
echo $DECIMALS | fmt -w $(($1 * 4))
