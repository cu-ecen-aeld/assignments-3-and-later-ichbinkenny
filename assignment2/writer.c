/***
 *
 * @author Kenneth Hunter
 * @description Program to create a file in a provided directory and then
 * 		write provided contents into the target.
 ***/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#define FILE_OPERATION_FAILURE -1
#define OPERATION_FAILURE 1

int cleanup(int status_code)
{
	closelog();
	return status_code;
}

int main(int argc, char** argv)
{
	// Setup syslog
	// Have output to console if we cannot open log, and also write to stderr on an error.
	openlog("Writer", LOG_CONS | LOG_PERROR,  LOG_USER);

	// Check that we have enough arguments to properly run
	if (argc < 2)
	{
		// Not enough arguments.
		syslog(LOG_ERR, "Either a path was not provided, or no data string was supplied.\n");
		syslog(LOG_ERR, "Usage: writer file_path data_string\n");
		
		return cleanup(OPERATION_FAILURE);
	}

	// Get the values for the target file name, and the data to write.
	const char* file_path = argv[1];
	const char* write_data = argv[2];

	syslog(LOG_DEBUG, "Writing %s to %s\n", write_data, file_path);

	// open the file in write mode. We are not reading the contents back,
	// so there is no need to request read as well.
	// We may also need to create the file.
	// Truncate the file if it already exists.
	int flags = O_WRONLY | O_CREAT | O_TRUNC;

	// Set our mode if the file needs to be created
	// to allow the owner to read and write, and everyone else
	// to read.
	int mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int file_descriptor = open(file_path, flags, mode);
	if (file_descriptor == FILE_OPERATION_FAILURE)
	{
		// Although we are told the provided path WILL exist,
		// we may not have write permissions to the file.
		// Or, some other error happened.
		syslog(LOG_ERR, "Failed to open the requested file.\n");
		return cleanup(OPERATION_FAILURE);
	}

	int write_status = write(file_descriptor, write_data, strlen(write_data));
	if (write_status < 0)
	{
		syslog(LOG_ERR, "Unable to write data to file.\n");
		return OPERATION_FAILURE;
	}

	return cleanup(0);

}
