#!/bin/bash

# Script editin file from interpreter to format:
# 1st out
# ...
# ith out
# File is in $1 parameter
# Author Michal Kukowski

if [ ! -f $1 ]; then
	echo "File not exist"
	exit
fi

# delete ? chars
sed -e s/\?\ //g -i $1

# delete > chars
sed -e s/\>\ //g -i $1

#delete 1-3 lines
sed -e '1,3d' -i $1

#delete last line
sed '$d' -i $1
