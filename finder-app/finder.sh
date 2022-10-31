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
	
	# We have a directory we can search. Now we want to find the total number of files inside.
	# Use ls to list all files, including the . and .. nodes, then pipe to word cound and get number
	# of lines read. Each line is one file.
	# -1 is for one line per file
	# May also need
	# -A is for almost all files (not . or ..)
	# -q to handle \n and other control characters properly. 
	filescount=$(ls -1A $filesdir | wc -l)
	
	# Get matching lines in each file.
	matchinglines=$(grep -R $searchstr $filesdir | wc -l)
	
	echo "The number of files are $filescount and the number of matching lines are $matchinglines"
else
	echo "Invalid directory provided. (Maybe you provided a file name instead of a directory?)"
	exit 1
fi

