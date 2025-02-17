#!/bin/bash
FILES=./tests/*

echo ""
echo "START"
echo ""

make clean
make

for FILE in $FILES
do
	./compile $FILE
	./optimize < tinyL.out > opt.out
	./run opt.out
	echo ""
done

echo ""
echo "END"
