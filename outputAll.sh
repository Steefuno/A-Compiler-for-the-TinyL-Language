#!/bin/bash
FILES=./tests/*

echo ""
echo "START"
echo ""

for FILE in $FILES
do
	./compile $FILE
	./optimize < tinyL.out
	echo ""
done

echo ""
echo "END"
