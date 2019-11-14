
/*
 *
 * Andrew Barthel
 * CS252 Shell Project
 *
 */

%{

#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "y.tab.h"
#include <iostream>
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

#endif
extern "C" char* read_line();

int mygetc(FILE* f) {

	static char* p;
	char ch;

	if (!isatty(0)) return getc(f);

	if (p == NULL || *p == 0) {

		char* s = read_line();
		p = s;

	}

	ch = *p;
	p++;

	return ch;

}
#undef getc
#define getc(f) mygetc(f)

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
    unput(c);
}

%}

%%

\n 	{
		return NEWLINE;
	}

";"	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}

">>&"	{
		return GREATGREATAND;
	}

">&"	{
		return GREATAND;
	}

">>"	{
		return GREATGREAT;
	}

">" 	{
		return GREAT;
	}

"<"	{
		return LESS;
	}

"&" {
		return AND;
	}

"|"	{
		return PIPE;
	}

"exit"	{
		std::cout << BOLD(FRED("Good bye!!\n"));
		_exit(0);
	}

"quit"	{
		std::cout << BOLD(FRED("Good bye!!\n"));
		_exit(0);
	}

\`[^\`]*\` {
		// Subshell.

		// Remove backticks
		char* string = strdup(yytext+1);
		string[strlen(string)-1]='\n';
		//printf("%s\n", string);

		// Set up temp string and pipes
		char buffer[1024];
		int pipeOne[2], pipeTwo[2]; //Double the pipes? double the fun!
		pipe(pipeOne), pipe(pipeTwo);
		
		//Housekeeping shit
		int tempIn=dup(0);
		int tempOut=dup(1);
		dup2(pipeOne[1], 1);
		dup2(pipeTwo[0], 0);
		close(pipeOne[1]);
		close(pipeTwo[0]);
		
		// Get ready world.
		int ret = fork(); //Deekeeds
		switch(ret) {
			case(-1): {

				perror("fork-subshell");
				exit(1);

			} case (0): {

				//Housekeeping
				dup2(pipeOne[0], 0);
				dup2(pipeTwo[1], 1);
				close(pipeOne[0]);
				close(pipeTwo[1]);
				
				// Set up for self call
				char* args[2];
				args[0] = strdup("/proc/self/exe"); //Call self
				args[1] = NULL;

				// Call self
				execvp(args[0], args);

				// You shouldn't be around these parts boy
				perror("execvp-subshell");
				exit(1);

			} default: {

				// Write to pipe. 
				//printf("%s\n", string);
				int length = strlen(string), i = 0;
				for(;i<length;i++) write(1,string+i,1);
				
				//Housekeeping
				dup2(tempOut, 1);
				close(tempOut);
				close(pipeTwo[1]);
				
				// Read from pipe.
				char* temp = buffer;
				char c = 0;
				while(read(0,&c,1)) {

					if(c=='\n') *temp = ' ';
					else *temp = c;
					temp++;

				} temp--;
				
				// Clear uneeded things
				while(temp>=buffer){unput(*temp); temp--;}
				
				// Final housecleaning
				dup2(tempIn, 0);
				close(tempIn);
				break;

			} 

		// Wait for all processes	
		} waitpid(ret,NULL,0);

	}
 
\"[^\"]*\" {
		// Remove quotes before inserting argument.
		yylval.string_val = (char*) malloc(sizeof(char)*(strlen(yytext) - 1));
		strncpy(yylval.string_val, yytext+1, strlen(yytext+1)-1);
		return WORDQUOTE;
	}

[^ ^|\t\n>][^ ^|\t\n>]*  {
		// Think of any other word possibilities.
		char* temp = yytext;
		int index = -1;
		int length = strlen(yytext);
		for(;temp[index++];index) {

			if(temp[index]==92) {

				int next = index-1;
				for(;next++<length;) temp[next] = temp[next+1];
				length--;

			}

		}
		temp[length]=0;
		/*//printf("%c\n%c\n", temp[0], temp[length-1]);
		if(temp[0]==39&&temp[length-1]==39) {
			yytext = temp;
			//printf("Y");
			goto JUMP;
		}*/
		yylval.string_val = (char*) malloc(sizeof(char)*(strlen(temp)+1));
		strcpy(yylval.string_val, temp);
		return WORD;
	}

.	{
		return NADA;
	}
