
/*
 * Andrew Barthel
 * CS252 Shell Project
 *
 */
#ifndef _COLORS_
#define _COLORS_

/* FOREGROUND */
#define RST  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define FRED(x) KRED x RST
#define FGRN(x) KGRN x RST
#define FYEL(x) KYEL x RST
#define FBLU(x) KBLU x RST
#define FMAG(x) KMAG x RST
#define FCYN(x) KCYN x RST
#define FWHT(x) KWHT x RST

#define BOLD(x) "\x1B[1m" x RST
#define UNDL(x) "\x1B[4m" x RST
#define ITAL(x) "\x1B[3m" x RST
#define	FRAM(x) "\x1B[51m" x RST

#endif
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <regex.h>
#include <pwd.h>
#include <vector>
#include "command.h"
enum COLOR { GOLD, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE};
COLOR color = WHITE;
void boilerUP();
void colorUsage();
// Variable to hold environment variables.
extern char** environ;
char* user = getenv("USER");
static const int _debug = 0;
// Create new instance of the ProcessArray.
std::vector<pid_t> process;

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if(_debug) printf("%s\n", argument);
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
}

void
SimpleCommand::expandEnv(char* argument) {

	// Expand environment variables.
	char* ptr = argument;
	int maxS = 1024; 
	int count = 0;
	char* expArg = (char*)calloc(maxS, sizeof(char));
	while(*ptr) {

		if(*ptr!='$') {

			expArg[count++]=*ptr;

		} else {

			ptr++;
			char* envVar = (char*)calloc(maxS,sizeof(char));
			int envCount = 0;
			if(*ptr=='{') {
				ptr++;
				while (*ptr!='}') {

					envVar[envCount]=*ptr++;
					envCount++;

				}
			}
			char* var = getenv(envVar);
			free(envVar);
			if(var!=NULL) {
				
				while(*var) {

					expArg[count]=*var;
					count++;
					var++;

				}

			} else {
				//TODO
				//perror("Environment Variable Expansion");
				//exit(1);
				//(ptr+envCount);
			}

		} ptr++;

	}
	Command::_currentSimpleCommand->insertArgument(strdup(expArg));
	free(expArg);
	return;
	
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
	if(_debug) print();
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile && _errFile != _outFile) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}
	
	// Print table for debug mode.
	if (_debug) print();

	// Perform the built-in commands - must be done in parent (except printenv).
	if (strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0) {

		//
		// CD COMMAND.
		//

		// Change directory accordingly.
		int dir;
		if (_simpleCommands[0]->_numberOfArguments == 1) {

			// Change directory to home.
			dir = chdir(getenv("HOME"));

		} else if (_simpleCommands[0]->_numberOfArguments == 2) {

			// Change directory to argumenr.
			dir = chdir(_simpleCommands[0]->_arguments[1]);

		} 

		// Error handling.
		if (dir != 0) perror("cd");
		clear(); prompt();
		return;

	} else if (strcmp(_simpleCommands[0]->_arguments[0], "setenv") == 0) {

		//
		// SETENV COMMAND.
		//
		
		// Set environment variable A to B.
		int set = setenv(_simpleCommands[0]->_arguments[1], _simpleCommands[0]->_arguments[2], 1);

		// Error handling.
		if (set != 0) perror("setenv");
		clear(); prompt();
		return;

	} else if (strcmp(_simpleCommands[0]->_arguments[0], "unsetenv") == 0) {

		//
		// UNSETENV COMMAND.
		//
		
		// Remove environment variable A from environment.
		int unset = unsetenv(_simpleCommands[0]->_arguments[1]);

		// Error handling.
		if (unset != 0) perror("unsetenv");
		clear(); prompt();
		return;

	} else if (strcmp(_simpleCommands[0]->_arguments[0], "gold") == 0) {

		// Change prompt to gold.
		clear();
		color = GOLD;
		prompt();
		return;

	} else if (strcmp(_simpleCommands[0]->_arguments[0], "yellow") == 0) {

		// Change prompt to yellow.
		clear();
		color = YELLOW;
		prompt();
		return;

	} else if (strcmp(_simpleCommands[0]->_arguments[0], "red") == 0) {

		// Change prompt to red.
		clear();
		color = RED;
		prompt();
		return;

	} else if (strcmp(_simpleCommands[0]->_arguments[0], "blue") == 0) {

		// Change prompt to blue.
		clear();
		color = BLUE;
		prompt();
		return;

	} else if (strcmp(_simpleCommands[0]->_arguments[0], "green") == 0) {

		// Change prompt to green.
		clear();
		color=GREEN;
		prompt();
		return;

	} else if (strcmp(_simpleCommands[0]->_arguments[0], "cyan") == 0) {

		// Change prompt to cyan.
		clear();
		color=CYAN;
		prompt();
		return;

	} else if (strcmp(_simpleCommands[0]->_arguments[0], "magenta") == 0) {

		// Change prompt to magenta.
		clear();
		color=MAGENTA;
		prompt();
		return;

	} else if (strcmp(_simpleCommands[0]->_arguments[0], "white") == 0) {

		// Change prompt to white.
		clear();
		color=WHITE;
		prompt();
		return;

	} else if (strcmp(_simpleCommands[0]->_arguments[0], "color") == 0) {

		// Print color usage.
		clear();
		colorUsage();
		prompt();
		return;
	} else if (strcmp(_simpleCommands[0]->_arguments[0], "boilerup")==0) {

		// BOILER THE FUCK UP
		clear();
		boilerUP();
		prompt();
		return;

	}

	// Save Open File instances of in, out, and err.
	int tempIn = dup(0);
	int tempOut = dup(1);
	int tempErr = dup(2);
	int fdin = 0;
	int fdout = 0;
	int fderr = 0;

	// START HANDLING COMMANDS!
	
	// Check if stdin is redirected.
	if (_inputFile) {
		
		// User specified a redirect. Try to open file.
		fdin = open(_inputFile, O_CREAT | O_WRONLY | O_TRUNC, 666);
		
		// The open syscall failed. Handle errors.
		if (fdin == -1) {
			FILE * error = fopen(_errFile, "a");
			fprintf(error, "Could not open input file.\n");
			fclose(error);
			exit(1);
		}

	} else {

		// The user did not specify a redirect.
		fdin = dup(tempIn);

	}

	// Check if stderr is redirected.
	if (_errFile) {

		// User specified a redirect.
		if (_append) fderr = open(_errFile, O_CREAT | O_WRONLY | O_APPEND, 600);
		else fderr = open(_errFile, O_CREAT | O_WRONLY | O_TRUNC, 0600);

		// Error handling.
		//TODO

	} else {

		// User did not specify redirect.
		fderr = dup(tempErr);

	}
	dup2(fderr, 2); close(fderr);

	// START EXECUTING COMMANDS.
	int ret; 
	for (int i = 0; i < _numberOfSimpleCommands; i++) {

		// Redirect input.
		dup2(fdin, 0);
		close(fdin); // Always remember to close!

		// Setup output.
		if (i == _numberOfSimpleCommands - 1) {

			// Last simple command - check if redirect.
			if (_outFile) {

				// Redirect specified - check if we append or create.
				if (_append) {
					fdout = open(_outFile, O_CREAT | O_WRONLY | O_APPEND, 600); 
				}
				else {
					fdout = open(_outFile, O_CREAT | O_WRONLY | O_TRUNC, 0600);
				}

			} else {

				// Use default output.
				fdout = dup(tempOut);

			} //TODO: ERROR FILE

		} else {

			// Not last simple command. Create pipe.
			int fdpipe[2];
			if (pipe(fdpipe) == -1) {

				// Pipe failed.
				perror("pipe");
				exit(2);

			}
			fdout = fdpipe[1];
			fdin = fdpipe[0];

		}

		// Redirect output && error
		dup2(fdout, 1);
		close(fdout); // Always remember to close!	

		// CHILD PROCESS - handle commands with pipes by creating children
		// always procreate - put on a rubber before you love her.
			
		// Create child process.
		ret = fork();
			
		// Code to run while in child.
		if (ret == 0) {
		
			// Print environment variables.
			int execF = 0;
			if (strcmp(_simpleCommands[i]->_arguments[0], "printenv") == 0) {

				//
				// PRINTENV COMMAND.
				//

				// Print the env variables.
				char** env = environ;
				while (*env != NULL) printf("%s\n", *env++);
				execF = 1;
				exit(1);

			}

			// Spawn off to run the command && set signal to be recieved.
			if (execF == 0) {
				signal(SIGINT, SIG_DFL);
				execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
				perror("execvp");
				_exit(1);
			}

		} else if (ret == -1) {

			perror("fork");
			exit(1);

		}

	}

	// Check if future zombie was bitten.
	if (_background == 1) process.push_back(ret);
	else waitpid(ret, NULL, 0);

	// Flush the toilet.
	fflush(stdout);

	// Restore in, out, and err.
	dup2(tempIn, 0);
	dup2(tempOut, 1);
	dup2(tempErr, 2);
	close(tempIn);
	close(tempOut);
	close(tempErr);

	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();

}

// Shell implementation

void
Command::prompt()
{
	if (isatty(0)) {
		switch(color){
			case(GREEN): {
				std::cout<<"\x1B[4m\x1B[32m"<<user<<"\x1B[0m";
				std::cout<<UNDL(FGRN(" : <msh> ~ $"));
				std::cout<<" ";
				break;
			} case(RED): {
                std::cout<<"\x1B[4m\x1B[31m"<<user<<"\x1B[0m";
				std::cout<<UNDL(FRED(" : <msh> ~ $"));
				std::cout<<" ";
				break;
			} case(BLUE): {
				std::cout<<"\x1B[4m\x1B[34m"<<user<<"\x1B[0m";
				std::cout<<UNDL(FBLU(" : <msh> ~ $"));
				std::cout<<" ";
				break;
			} case(YELLOW): {
				std::cout<<"\x1B[4m\x1B[33m"<<user<<"\x1B[0m";
				std::cout<<UNDL(FYEL(" : <msh> ~ $"));
				std::cout<<" ";
				break;
			} case(MAGENTA): {
				std::cout<<"\x1B[4m\x1B[35m"<<user<<"\x1B[0m";
				std::cout<<UNDL(FMAG(" : <msh> ~ $"));
				std::cout<<" ";
				break;
			} case(GOLD): {
				std::cout << "\x1B[4m\x1B[33m" << user << " : <msh> ~ $" << "\x1B[0m";
				std::cout << " ";
				break;
			} case(CYAN): {
				std::cout<<"\x1B[4m\x1B[36m"<<user<<"\x1B[0m";
				std::cout<<UNDL(FCYN(" : <msh> ~ $"));
				std::cout<<" ";
				break;
			} case(WHITE): {
				;
			} default: {
				std::cout<<"\x1B[4m\x1B[37m"<<user<<"\x1B[0m";
				std::cout<<UNDL(FWHT(" : <msh> ~ $"));
				std::cout<<" ";
				break;
			}
		} fflush(stdout);
	}
}

void colorUsage() {

	// Print color usage.
	std::cout<<FYEL("color ")<<FRED("command ")<<FGRN("usage: \n");
	std::cout<<FBLU("\tChange ")<<FMAG("the ")<<FCYN("prompt ")<<FWHT("color: \n");
	std::cout<<FRED("\t\tred:\tChanges prompt to red.\n");
	//std::cout<<FYEL("\t\tyellow:\tChanges prompt to yellow.\n");
	std::cout<<FGRN("\t\tgreen:\tChanges prompt to green.\n");
	std::cout<<FBLU("\t\tblue:\tChanges prompt to blue.\n");
	std::cout<<FMAG("\t\tmagenta:Changes prompt to magenta.\n");
	std::cout<<FCYN("\t\tcyan: \tChanges prompt to cyan.\n");
	std::cout<<FWHT("\t\twhite:\tChanges prompt to white.\n");
	std::cout<<"\x1B[33m"<<"\t\tgold :\tChanges prompt to gold.\n\n"<<"\x1B[0m";
	fflush(stdout);

}

void boilerUP() {
		std::cout<<"\x1B[43m\x1B[30m"<<"\n\t\tHAIL PURDUE!\n"<<"\x1b[0m";
		std::cout<<"\x1B[40m\x1B[33m"<<"To your call once more we rally,\n"<<"\x1b[0m";
        std::cout<<"\x1B[43m\x1B[30m"<<"Alma mater hear our praise.\n"<<"\x1b[0m";
		std::cout<<"\x1B[40m\x1B[33m"<<"Where the Wabash spreads its valley;\n"<<"\x1b[0m";
		std::cout<<"\x1B[43m\x1B[30m"<<"Filled with joy our voices raise.\n"<<"\x1b[0m";
		std::cout<<"\x1B[40m\x1B[33m"<<"From the skies in swelling echos\n"<<"\x1b[0m";
		std::cout<<"\x1B[43m\x1B[30m"<<"Come the cheers that tell the tale\n"<<"\x1b[0m";
		std::cout<<"\x1B[40m\x1B[33m"<<"Of your victories and your heroes.\n"<<"\x1b[0m";
		std::cout<<"\x1B[43m\x1B[30m"<<"Hail Purdue! We sing all hail!\n\n"<<"\x1b[0m";
		std::cout<<"\x1B[40m\x1B[33m"<<"Hail, hail to old Purdue!\n"<<"\x1b[0m";
		std::cout<<"\x1B[43m\x1B[30m"<<"All hail to our old gold and black!\n"<<"\x1B[0m";
        std::cout<<"\x1B[40m\x1B[33m"<<"Hail, hail to old Purdue!\n"<<"\x1B[0m";
		std::cout<<"\x1B[43m\x1B[30m"<<"Our friendship may she never lack.\n"<<"\x1B[0m";
		std::cout<<"\x1B[40m\x1B[33m"<<"Ever grateful, ever true,\n"<<"\x1B[0m";
		std::cout<<"\x1B[43m\x1B[30m"<<"Thus we raise our song anew\n"<<"\x1B[0m";
		std::cout<<"\x1B[40m\x1B[33m"<<"Of the days we spent with you,\n"<<"\x1B[0m";
		std::cout<<"\x1B[43m\x1B[30m"<<"All hail our own Purdue!\n\n"<<"\x1B[0m";
		std::cout<<"\x1B[40m\x1B[33m"<<"When in after years we're turning,\n"<<"\x1B[0m";
		std::cout<<"\x1B[43m\x1B[30m"<<"Alma mater, back to you,\n"<<"\x1B[0m";
		std::cout<<"\x1B[40m\x1B[33m"<<"May our hearts with love be yearning\n"<<"\x1B[0m";
		std::cout<<"\x1B[43m\x1B[30m"<<"For the scenes of old Purdue.\n"<<"\x1B[0m";
		std::cout<<"\x1B[40m\x1B[33m"<<"Back among your pathways winding\n"<<"\x1B[0m";
		std::cout<<"\x1B[43m\x1B[30m"<<"Let us seek what lies before,\n"<<"\x1B[0m";
		std::cout<<"\x1B[40m\x1B[33m"<<"Fondest hopes and aims e'er finding,\n"<<"\x1B[0m";
		std::cout<<"\x1B[43m\x1B[30m"<<"While we sing of days of yore.\n\n"<<"\x1B[0m";
		fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

// Handle SIGINT from Ctrl+C - IGNORE IT!
void sigIntHandler(int signal) {
	// KILL DEE KEEDS!
	for (int i = 0; i < process.size(); i++) kill(process[i], SIGINT);
	process.clear();
	Command::_currentCommand.clear();
	printf("\n");
	Command::_currentCommand.prompt();
}  

// Handle SIGCHLD from children processes.
void sigChildHandler(int signal) {
	int pid = -1;
	int i = 0;
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
		bool find = false;
		while (i < process.size()) {
			if (process[i] == pid) {
				find = true;
				break;
			}
			i++;
		}
		if (find) {
			std::cout << "[" << pid << "]" << " exited." << std::endl;
			Command::_currentCommand.prompt();
		}
	}
}

int main(int argc, char** argv)
{
	// Set up SIGINT handler for Ctrl+C.
	struct sigaction sigIntAction;
	sigIntAction.sa_handler = sigIntHandler;
	sigemptyset(&sigIntAction.sa_mask);
	sigIntAction.sa_flags = SA_RESTART;
	int error1 = sigaction(SIGINT, &sigIntAction, NULL);
	if (error1 == -1) {
		perror("sigaction: SIGINT");
		exit(6);
	}

	// Set up SIGCHILD handler for child processes.
	struct sigaction sigChildAction;
	sigChildAction.sa_handler = sigChildHandler;
	sigemptyset(&sigChildAction.sa_mask);
	sigChildAction.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	int error2 = sigaction(SIGCHLD, &sigChildAction, NULL);
	if (error2 == -1) {
		perror("sigaction: SIGCHLD");
		exit(7);
	}

	// Enable debug mode.
//	if (argc == 2 && strcmp(argv[1], "--debug") == 0) _debug = 1;

	Command::_currentCommand.prompt();
	yyparse();

}













