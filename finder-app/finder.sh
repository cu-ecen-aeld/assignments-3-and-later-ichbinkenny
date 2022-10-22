#!/bin/bash
filesdir=$1
searchstr=$2
argcount=$#

if [ $argcount -lt 2 ] 
then
	echo "No files dir provided, or search string missing."
	echo "Usage: finder.sh search_path search_params"
	exit 1
elif [ -d $filesdir ]
then

	echo "Directory exists!"
else
	echo "Invalid directory provided. (Maybe you provided a file name instead of a directory?)"
	exit 1
fi

