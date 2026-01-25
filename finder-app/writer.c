/******************************************************************************
 * Assignment 2
 *
 * File: writer.c
 *
 * Description:
 *   This program takes a file path and a string as command-line arguments
 *   and writes the string into the specified file. 
 *   If the file does not exist, it will be created. If it exists, it will be 
 *   truncated before writing. Errors are logged using syslog.
 *
 * Author: Dhigvijay Mohan
 * Date  : 25-01-2026
 *
 * Acknowledgements:
 *   Usage of syslog referenced from:
 *   https://stackoverflow.com/questions/8485333/syslog-command-in-c-code
 *   Linux System Programming by Robert Love Chapter 2
 *
 ******************************************************************************/

#include <stdio.h>      // Standard I/O functions
#include <sys/types.h>  // Data types used in system calls
#include <sys/stat.h>   // File mode constants
#include <fcntl.h>      // File control definitions
#include <unistd.h>     // POSIX API (write, close)
#include <string.h>     // String handling (strlen)
#include <syslog.h>     // System logging
#include <errno.h>      // Error numbers

// Macro for file permission mode (owner read/write, group/others read)
#define FILE_MODE 0644

int main(int argc, char *argv[])
{
    // Open a connection to the system logger
    // LOG_PID: Include PID with each message
    // LOG_PERROR: Also print messages to stderr
    // LOG_USER: User-level messages
    openlog("writer", LOG_PID | LOG_PERROR, LOG_USER);

    // Check for proper number of command-line arguments
    if (argc < 3) {
        syslog(LOG_ERR, "Missing arguments or invalid arguments");
        printf("Error: Missing arguments.\n");
        printf("Usage: %s <inputfile> <inputstr>\n", argv[0]);
        closelog();
        return 1;
    }

    // Store input arguments
    const char *inputfile = argv[1];
    const char *inputstring = argv[2];

    // Open the file for writing
    // O_WRONLY  : Open for writing only
    // O_CREAT   : Create file if it does not exist
    // O_TRUNC   : Truncate file to zero length if it exists
    // 0644      : File permission (owner read/write, group/others read)
    int fd = open(inputfile, O_WRONLY | O_CREAT | O_TRUNC, FILE_MODE);

    if (fd == -1) {
        perror("Open");  // Print error to stderr
        syslog(LOG_ERR, "Failed to open file %s: %s", inputfile,strerror(errno));
        closelog();
        return 1;
    }

    // Write the input string to the file
    ssize_t nr = write(fd, inputstring, strlen(inputstring));
    if (nr == -1) {
        perror("Write");
        syslog(LOG_ERR, "Failed to write to file %s: %s", inputfile,strerror(errno));
        close(fd);
        closelog();
        return 1;
    }

    syslog(LOG_DEBUG, "Writing to file successful");

    // Close the file descriptor
    if (close(fd) == -1) {
        perror("Close");
        syslog(LOG_ERR, "Error closing file %s: %s", inputfile,strerror(errno));
        closelog();
        return 1;
    }

    // Close the connection to the syslog
    closelog();

    return 0;
}

