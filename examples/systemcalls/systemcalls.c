#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
	// If cmd is NULL, there is no command to execute, so return false
	if (cmd == NULL) {
        return false;
    }

	// Call the system() function to execute the command
    // system() returns:
    //   -1    → if there was an error invoking the shell
    //   otherwise → exit status of the command executed
    int status = system(cmd);

    // If system() failed to execute the shell, return false
    if (status == -1) {
        return false;
    }


    // WIFEXITED(status) checks if the command terminated normally
    // WEXITSTATUS(status) gets the exit code of the command
    // Return true only if the command exited normally with status 0
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return true;
    }

	// If the command did not exit normally or exit code was non-zero, return false
    return false;	
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    va_end(args);

	// Fork a new process
    pid_t pid = fork();

    if (pid == -1) {
        // fork failed → cannot create child process
        return false;
    }

    if (pid == 0) {
        // CHILD PROCESS
        // Execute the command with execv
        // execv replaces the child process memory with the new program
        // It only returns on failure
        execv(command[0], command);

        // If execv failed, exit child with non-zero status
        exit(1);
    }

    // parent process
	// wait for the child process to finish
    int status;
    if (waitpid(pid, &status, 0) == -1) {
		//wait pid failed
        return false;
    }

	// Check if child exited normally and returned 0
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return true;
    }

	// Child exited with non-zero status or abnormal termination
    return false;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    va_end(args);

	// Fork a new process
    pid_t pid = fork();

	// Use switch-case to handle fork result
	switch (pid) {
    	case -1: // fork failed
		{
			perror("fork");
			return false;
		}
		case 0: //child process 
		{
			// Open the output file for writing (create/truncate)
    	    int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    	    if (fd == -1) {
				perror("open");
    	        exit(1);
    	    	close(fd);
			}

    	    // Redirect stdout to the file
    	    if (dup2(fd, STDOUT_FILENO) < 0) {
    	        perror("dup2");
				close(fd);
    	        exit(1);
    	    }

    	    close(fd); // Close original fd; stdout now points to the file

			// Execute the command
    	    execv(command[0], command);

    	    // execv only returns on failure
    	    perror("execv");
			exit(1);
    	}
		default:
    	{
			// parent process
    		int status;
			// Wait for child process to finish
    		if (waitpid(pid, &status, 0) == -1) {
    		    return false;
    		}

			// Check if child exited normally with exit code 0
			if (WIFEXITED(status) && WEXITSTATUS(status) == 0 )  {
    		    return true;
    		}
    		
			// Child exited abnormally or with non-zero status
			return false;
		}
	}

}
