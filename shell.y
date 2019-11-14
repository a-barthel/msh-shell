
/*
 * 
 * Andrew Barthel
 * CS252 Shell Project
 *
 */

%token	<string_val> WORD WORDQUOTE 

%token 	NADA GREAT NEWLINE GREATGREAT GREATGREATAND GREATAND LESS AND PIPE  

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#include <regex.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "command.h"
void yyerror(const char * s);
int yylex();
int _debug = 0;
void expandTilde(char* arg);
void expandEnv(char* arg);
void subShell(char* arg);
void expandWild(char* prefix, char* suffix);
void parseArg(char* arg);
int wildCount = 0;
char** wildArray;
int maxWEntries = 5; // 5 by Default - wildArray will start with 5
		     // entries and will be a resizeable array.  The
		     // array will double maxWEntries every time wildCount
		     // is equal to maxWEntries. O(n log n) for insert.
int flag = 0;
int compare (const void* strOne, const void* strTwo);
%}

%%

goal:	
	commands
	;

commands: 
	commands command
	| command 
	;

command: 
	simple_command 
        ;

simple_command:	
	pipe_list iomodifier_list zombiekiller_optional  {
		if(_debug) printf("Yacc: execute()\n");
		//Did nline thing 
		Command::_currentCommand.execute();
	}
	| NEWLINE { Command::_currentCommand.prompt(); }
	| error NEWLINE { yyerrok; Command::_currentCommand.prompt(); }
	;

pipe_list:
	pipe_list PIPE command_and_args
	| command_and_args
	;

command_and_args:
	command_word arg_list {
		if(_debug) printf("Yacc: insertSimpleCommand()\n");
		Command::_currentCommand.insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| 
	;

argument:
	WORD {
	      parseArg($1);
	}
	| WORDQUOTE {
		Command::_currentSimpleCommand->insertArgument($1);
	}
	;

command_word:
	WORD {
	       if(_debug) printf("Yacc: insertArgument() as command\n");
	       //char* temp = strdup($1);
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	       //free(temp);
	}
	| //New
	;

iomodifier_list:
	iomodifier_list iomodifier_opt
	| 
	;

iomodifier_opt:
	GREAT WORD {
		if(_debug) printf("Yacc: output redirect\n");
		if (Command::_currentCommand._outFile) perror("Ambiguous output redirect.\n");
		Command::_currentCommand._outFile = $2;
	}
	| LESS WORD {
		if(_debug) printf("Yacc: input redirect\n");
		if (Command::_currentCommand._inputFile) perror("Ambiguous input redirect.\n");
		Command::_currentCommand._inputFile = $2;
	} 
	| GREATAND WORD {
		if(_debug) printf("Yacc: output & error redirect\n");
		if (Command::_currentCommand._outFile) perror("Ambiguous output redirect.\n");
		if (Command::_currentCommand._errFile) perror("Ambiguous error redirect.\n");
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;
	}
	| GREATGREAT WORD {
		if(_debug) printf("Yacc: output append redirect\n");
		if (Command::_currentCommand._outFile) perror("Ambiguous output redirect.\n");
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._append = 1;
	}
	| GREATGREATAND WORD {
		if(_debug) printf("Yacc: output & error append redirect\n");
		if (Command::_currentCommand._outFile) perror("Ambiguous output redirect.\n");
		if (Command::_currentCommand._errFile) perror("Ambiguous error redirect.\n");
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;
		Command::_currentCommand._append = 1;
	}
	;

zombiekiller_optional:
	AND NEWLINE {
		if(_debug) printf("Yacc: background process\n");
		Command::_currentCommand._background = 1;
	}
	| NEWLINE 
	;

%%

void parseArg(char* arg) {

	// We will try to parse argument one more time before inserting.

	// Check if word is an environment variable.
	char* check = arg;
	if (strchr(check, '$')) expandEnv(arg); // Doesn't have to be beginning - change this later!

	// Check if word is a tilde expansion.
	else if (*check == '~') expandTilde(arg);

	// Check if not wildcard.
	else if (!strchr(check, '*') && !strchr(check, '?')) Command::_currentSimpleCommand->insertArgument(arg);

	//if (_debug) printf("Yacc: insertArgument() as argument\n");
	// Arg must be a wildcard.
	else {
		if(*arg=='/') expandWild("/", arg+1);
		else expandWild(NULL, arg);
		if (!wildCount) Command::_currentSimpleCommand->insertArgument(strdup(arg));
		// Sort if needed.
		//printf("%d\n", wildCount);
		if (wildCount > 1) qsort(wildArray, wildCount, sizeof(const char*), compare);
		for (int i = 0;i < wildCount;i++) Command::_currentSimpleCommand->insertArgument(strdup(wildArray[i]));
		// Clean up everything for next command && free allocated memory.
		free(wildArray);
		wildCount = 0;
		flag = 0;
		maxWEntries = 5; // Set back to default! (not zero)
		wildArray = NULL; 

	}
	return;

}

int compare(const void* strOne, const void* strTwo) { return strcmp( *(const char**)strOne, *(const char**)strTwo); }

void expandTilde(char* arg) {
	
	// Expand tilde.

	struct passwd *pw;
	char* exp;
	char* user = getenv("USER");
	char* check = arg;
	pw = getpwuid(getuid());
	exp = strdup(pw->pw_dir);
	// Checks for ~ , ~/, and ~user.
	if (*(check+1) == '\0' || (*(check+1) == '/' && *(check+2) == '\0') || strcmp(user, (check+1)) == 0) {
		//printf("NO\n");
		Command::_currentSimpleCommand->insertArgument(exp);
	} else if (strlen(check) > (strlen(user)+1) && strncmp((check+1), user, strlen(user)) == 0) {
		// Checks for ~user/path
		//printf("NO\n");
		char* dir = pw->pw_dir;
		char* subdir = strchr(check, '/');
		//printf("%s\n", subdir);
		char* path = (char*)malloc(sizeof(char)*(strlen(dir)+strlen(subdir)+1));
		path[0] = 0;
		strcat(path, dir);
		strcat(path, subdir);
		Command::_currentSimpleCommand->insertArgument(path);
	} else if (strstr(check, exp) == NULL && *(check+1) == '/') {
		// Checks for ~/pathSpawnedOffHome
		//printf("NO\n");
		char* dir = pw->pw_dir;
		char* subdir = (check+1);
		char* path = (char*)malloc(sizeof(char)*(strlen(dir)+strlen(subdir)+1));
		path[0] = 0;
		strcat(path, dir);
		if (dir[strlen(dir)-1] != '/' && *subdir != '/') {
			path = (char*)realloc(path, sizeof(char)*(strlen(dir)+strlen(subdir)+2));
			strcat(path, "/");
		}
		strcat(path, subdir);
		Command::_currentSimpleCommand->insertArgument(path);
	} else if (strcmp((check+1), user) != 0 && strchr((check+1), '/') == NULL) {
		// Checks ~otherUser
		//printf("NO\n");
		pw = getpwnam((check+1));
		char* dir = pw->pw_dir;
		//printf("%s\n", dir);
		Command::_currentSimpleCommand->insertArgument(strdup(dir));
	} else if (strncmp((check+1), user, strlen(user)) != 0 && strchr((check+1), '/') != NULL) {
		// Checks ~otherUser/subdir
		int i = 0;
		while (check[i] != '/') i++;
		char* oUser = (char*)malloc(sizeof(char)*i);
		strncpy(oUser, (check+1), i-1);
		oUser[i] = 0;
		pw = getpwnam(oUser);
		char* dir = pw->pw_dir;
		//printf("%s\n", dir);
		char* subdir = strchr((check+1), '/');
		//printf("%s\n", subdir);
		char* path = (char*)malloc((strlen(dir)+strlen(subdir)+10)*sizeof(char));
		path[0] = 0;
		strcat(path, dir);
		if (dir[strlen(dir)-1] != '/' && *subdir != '/') {
			path = (char*)realloc(path, sizeof(char)*(strlen(dir)+strlen(subdir)+20));
			strcat(path, "/");
		}
		strcat(path, subdir);
		//printf("%s\n", path);
		Command::_currentSimpleCommand->insertArgument(path);
	} else {
		// Checks rest.
		//printf("NO\n");
		Command::_currentSimpleCommand->insertArgument(strdup((check+1)));
	}
	return;
}

void expandEnv(char* arg) {
	
	// Expand and check if env variable.
	Command::_currentSimpleCommand->expandEnv(strdup(arg));
	return;
}

void expandWild(char* prefix, char* suffix) {

	// Perform Wildcard expansion.
	//printf("%s\n", prefix);
	//if(*suffix=='/') *prefix = '/';
	if (wildArray == NULL) {
		
		// Initialize array.
		wildArray = (char**)malloc(maxWEntries*sizeof(char*));

	}

	if (!*suffix) {

		// Suffix is empty - insert prefix!
		
		
		// Resize the mildArray if needed.
		if (maxWEntries == wildCount) {
		
			// Just realloc with new maxWEntries - don't copy - realloc retains memory.
			maxWEntries <<= 1;
			wildArray = (char**)realloc(wildArray, maxWEntries*sizeof(char*));
		
		}

		// Insert prefix now.
		if(prefix[strlen(prefix)-1]=='/') prefix[strlen(prefix)-1] = 0;
		wildArray[wildCount] = strdup(prefix);
		wildCount++;
		return;

	}
	
	// Obtain next component in suffix.
	char* s = strchr(suffix, '/');
	char* component = (char*)calloc(1024, sizeof(char)); // Use calloc to ensure null-termination.
	if (s) {

		//Copy to first '/'
		if(s-suffix) strncpy(component, suffix, s - suffix);
		suffix = s +1;

	} else {

		// Last part of path - copy rest.
		strcpy(component, suffix);
		suffix = suffix + strlen(suffix);

	}

	// Expand the component.
	char* newPrefix = (char*)calloc(1024, sizeof(char));
	if (strchr(component, '*') == NULL && strchr(component, '?') == NULL) {
	
		// No wildcards.
		if (!*prefix && *component) { // Handle null prefix.

			strcpy(newPrefix, component);

		} else if (*component) { // Non-null prefix.

			strcpy(newPrefix, prefix);
			//strcat(newPrefix, "/");
			strcat(newPrefix, component);

		}

		// Recursively call expandWild
		strcat(newPrefix, "/");
		if(*component) expandWild(newPrefix, suffix);
		else expandWild("", suffix);

		// Free and return after many recursive calls.
		free(component); free(newPrefix);
		return;

	}

	// Component has wildcards.
	// Convert to regular expression.
	char* rgx = (char*)calloc(2*strlen(component)+10, sizeof(char));
	char* ptr = rgx;
	char* temp = component;
	
	// Conform to regex syntax.
	*(ptr++) = '^';
	while (*temp) {

		// Do weird regex things. Do things.
		switch (*temp) {
			
			case ('*'):
				*(ptr++) = '.';
				*(ptr++) = '*';
				break;
			case ('?'):
				*(ptr++) = '.';
				break;
			case ('.'):
				*(ptr++) = '\\'; 
				*(ptr++) = '.';
				break; 
			case ('/'):
				break;
			default:
				*(ptr++) = *temp;

		} temp++; // Forgot to do this....kinda important - I think this was the bug. FUCK THIS.

	}

	// Null terminate.
	*(ptr++) = '$'; *(ptr++) = 0;  // Try to take this out later - I used calloc...

	// Compile REGEX.
	regex_t regex;
	if (regcomp(&regex, rgx, REG_EXTENDED|REG_NOSUB)) {

		// Error with regex.
		perror("regex");
		exit(1);

	}

	// FINALLY TIME FOR THE FIRST TERNARY OF THE SEMESTER
	// if prefix is null = '.' else prefix
	char* tempDir = (prefix) ? (prefix) : (char*)".";
	//printf("%s\n", tempDir);
	DIR* dir = opendir(tempDir);
	if (!dir) {

		// Coundn't open directory.
		free(rgx); free(component); free(newPrefix); 
		return;

	}

	// Try to open directory/subdirectory.
	struct dirent* entry;
	while (entry = readdir(dir)) {

		// Find match.
		if (!regexec(&regex, entry->d_name, 0, NULL, 0)) {
			
			// Format.
			if (!prefix || !*prefix) strcpy(newPrefix, entry->d_name);
			else {
				//strcpy(newPrefix, prefix);  
				//strcat(newPrefix, entry->d_name);
				sprintf(newPrefix, "%s%s/", prefix, entry->d_name);
			}
			//strcat(newPrefix, "/");
			// Expand accordingly.
			if (entry->d_name[0] == '.') {
				if(*component == '.') expandWild(newPrefix, suffix);
			}//else if (*component != '.') continue;
			else expandWild(newPrefix, suffix);

		}

	}

	//free(rgx); free(component); free(newPrefix);
	//delete regex;
	return;

}

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif



