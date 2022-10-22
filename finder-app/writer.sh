#!/bin/bash
output_file=$1
write_string=$2

if [ $# -lt 2 ] 
then
	echo "Either no file was specified, or no write data was provided."
	echo "Usage: writer.sh absolute_file_path file_data"
	exit 1
fi

# Check if the file exists. If not, we can check for write permissions and create it.
if [ ! -e $output_file ] 
then
	# See if we can create the file.
	# Does the directory exist?
	directory_name=$(dirname "$output_file")
	if [ ! -d directory_name ]
	then
		# Looks like the directory doesn't exist. 
		# Let's make one
		dir_creation_status=$(mkdir -p $directory_name)
		if [ $? -ne 0 ]
		then
			# Creating the directory failed.
			# Exit with an error.
			echo "Failed to create the directory for the provided file ($directory_name)."
			exit 1
		fi
	fi
	file_creation_status=$(echo $write_string > $output_file)
	if [ $? -ne 0 ]
	then
		echo "Failed to write to file: $output_file."
		exit 1
	fi
fi
