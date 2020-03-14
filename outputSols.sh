#!/bin/bash
FILES=./tests/*

echo ""
echo "START"
echo ""

for FILE in $FILES
do
	./compile.sol $FILE
	./optimize.sol < tinyL.out
	echo ""
done

echo ""
echo "END"
