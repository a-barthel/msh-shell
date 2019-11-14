/*
*   Andrew Barthel
*   CS252 Shell Project
*/

#include <regex.h>
#include <dirent.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#define MAX_BUFFER_LINE 2048

char** wildArray;
int wildCounts = 0;
int maxWEntry = 5;
// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];
char** history;
int history_length = 0;

void read_line_print_usage() {

    // Print the usage of the program 

    char * usage = "\n"
    " ctrl-?       Print usage\n"
    " left arrow   move cursor to left\n"
    " right arrow  move cursor to right\n"
    " ctrl-A	   move cursor to start of line\n"
    " ctrl-E	   move cursor to end of line\n"
    " ctrl-D	   Deletes char at cursor\n"
    " Backspace    Deletes last character\n"
    " down arrow   See next command in the history\n"
    " up arrow     See last command in the history\n";

    write(1, usage, strlen(usage));

}


/* 
* Input a line with some basic editing.
*/
char * read_line() {

    // Save terminal and set to raw mode.
    struct termios saveTemp;
    tcgetattr(0,&saveTemp);
    tty_raw_mode();

    line_length = 0;
    int index = 0;
    int history_index = 0;

    // Read one line until enter is typed
    while (1) {

        // Read one character in raw mode.
        char ch;
        read(0, &ch, 1);

        if (ch>=32&&ch<=126&&ch!=33) {
            // It is a printable character. 

            // Quick-fix if not ! command
            print:

            // Do echo
            write(1, &ch, 1);

            // Check max chars
            if (line_length == MAX_BUFFER_LINE-2) break;

            // Move chars
            int i = line_length+1;
            for(; i > index;i--) line_buffer[i] = line_buffer[i-1];

            line_buffer[index] = ch;
            line_length++; index++;

            // reprint
            for(i = index; i < line_length; i++) {

                ch = line_buffer[i];
                write(1, &ch, 1);

            }

            ch = ' ';
            write(1, &ch, 1);
            ch = 8;
            write(1, &ch, 1);

            // Get back to index
            for (i = index; i < line_length; i++) {

                // Simulate left arrow
                ch = 27;
                write(1, &ch, 1);
                ch = 91;
                write(1, &ch, 1);
                ch = 68;
                write(1, &ch, 1);

            }

        } else if (ch==10) {
            
            // <Enter> was typed. Return line

            // Add to history.
            if (!history_length) history = malloc(sizeof(char*)*100);
            history[history_length] = calloc(MAX_BUFFER_LINE, sizeof(char));
            strncpy(history[history_length], line_buffer, line_length);
            history_length++;
            // Print newline
            write(1,&ch,1);

            break;

        } else if (ch == 31) {
            
            // ctrl-? - print usage
            read_line_print_usage();
            line_buffer[0]=0;
            break;

        } else if (ch == 1) {
            
            // BABY IM HOME - Ctrl-A

            // Check if nothing typed or already at begining
            if (!line_length || !index) continue;

            int i = 0;
            for (; i++ < index;) {

                // Simulate left arrow
                ch = 27;
                write(1, &ch, 1);
                ch = 91;
                write(1, &ch, 1);
                ch = 68;
                write(1, &ch, 1);

            } index = 0;

        } else if (ch == 5) {

            // BABY IM HOME LOL JK IM AT THE END OF THE LINE
            // Ctrl-E

            // Check if nothing typed or already at end
            if (!line_length || index==line_length) continue; 
            
            while (index != line_length) {

                // Simulate right arrow.
                ch=27;
                write(1,&ch,1);
                ch=91;
                write(1,&ch,1);
                ch=67;
                write(1,&ch,1);
                index++;

            }

        } else if (ch == 8 || ch == 127) {
            // <backspace> was typed. Remove previous character read.
            
            // Check if nothing left or at beginning
            if (!line_length || !index) continue;

            // Save buffer minus last char.
            int i = index;
            for (; i < line_length; i++) line_buffer[i-1] = line_buffer[i];
            line_length--;
            index--;

            // Write a space to erase the last character read
            ch = ' ';
            write(1,&ch,1);

            // Go back one character
            ch = 8;
            write(1,&ch,1);
            
            // Go back one character
            ch = 8;
            write(1,&ch,1);

            // Clean line up to point of deletion
            i = index;
            while (i < line_length) {

                ch = line_buffer[i];
                write(1, &ch, 1);
                i++;

            }

            // Print space
            ch = ' ';
            write(1,&ch,1);

            // Go back one character
            ch = 8;
            write(1,&ch,1);
            
            i = index;
            for (; i < line_length;i++) {

                // Simulate left arrow
                ch = 27;
                write(1, &ch, 1);
                ch = 91;
                write(1, &ch, 1);
                ch = 68;
                write(1, &ch, 1);

            }   

        } else if (ch == 4) {

            // Ctrl-D

            // Afterthought quick-fix for Delete - will fix later
            ctrlD:

            // Check if at end of line
            if(index==line_length) continue;
            
            // Print space
            ch = ' ';
            write(1,&ch,1);

            // Go back one character
            ch = 8;
            write(1,&ch,1);
            
            // Move all chars after index up one.
            int i = index;
            for(; i < line_length-1;i++) line_buffer[i] = line_buffer[i+1];
            line_length--;
            
            // Write changes
            i = index;
            for(; i < line_length;i++) {

                ch = line_buffer[i];
                write(1,&ch,1);

            }

            // Print space
            ch = ' ';
            write(1,&ch,1);

            // Go back one character
            ch = 8;
            write(1,&ch,1);

            i = index;
            for (; i++ < line_length;) {

                // Simulate left arrow
                ch = 27;
                write(1, &ch, 1);
                ch = 91;
                write(1, &ch, 1);
                ch = 68;
                write(1, &ch, 1);

            }  

        } else if (ch==33) {
            
            //Run last command

            // Check if not beginning of line.
            if(index || history_length == 0) goto print;

            // Check if any history
            if(!history_length) perror("no history");
            
            // Delete line
            int i = 0;
            for(; i++ < line_length;) {
                
                ch=8;
                write(1,&ch,1);

            } i = 0;

            // Clear with spaces
            for(;i++<line_length;) {
                
                ch=' ';
                write(1,&ch,1);

            } i = 0;
            
            // Clean line/
            for(;i++<line_length;) {
                ch=8;
                write(1,&ch,1);
            } 

            // Adjust history index accordingly && copy
            if(history_index<history_length) history_index++;
            strcpy(line_buffer,history[history_length-history_index]);
            line_length=strlen(line_buffer);
            
            // Let end part handle enter and null char
            break;

        } else if (ch==9) {
            int breakF = 0;
            // Tab completion. BEWARE this code will hurt the eyes

            // Find string to complete.
            int i = 0;
            for(; line_buffer[i++]!=32;);

            // Make sure not end.
            if(i>=line_length) continue;

            // Save string
            char* expand = (char*)calloc(1024, sizeof(char));
            int j = 0;
            for(; i<line_length;) expand[j++] = line_buffer[i++];

            // Convert to wild card
            expand[strlen(expand)] = '*';

            // Try to expand.
            expandTab(NULL, expand);

            switch(wildCounts) {

                case(0): continue;
                
                case(1): {

                    // Find spot to start expansion
                    expand[strlen(expand)]=0;
                    int i = 0;
                    // This next line is majik
                    for(;expand[i++]==*(wildArray[0])++;);
                    
                    // Print rest of characters.
                    int j = 0;
                    *(wildArray[0])--; *(wildArray[0])--;
                    for(; *(wildArray[0])++!=0;) {

                            ch=*(wildArray[0]);
                            line_buffer[line_length++] = ch;
                            index++;
                            write(1,&ch,1);

                    }
                    //line_length++; index++;
                    breakF = 1;
                    break;

                }
                
                default: {
                    
                    // Run echo <expand>*
                    char* run = (char*)calloc(1024, sizeof(char));;
                    strcat(run, "echo ");
                    strcat(run, expand);
                    strcpy(line_buffer, run);
                    line_length = strlen(expand)+5;
                    break;
                    // // Append enter and null character
                    // line_buffer[line_length]=10;
                    // line_length++;
                    // line_buffer[line_length]=0;

                    // // Restore attributes and return
                    // tcsetattr(0,TCSANOW,&saveTemp);
                    // return line_buffer;

                }
                continue;
            } 

            free(wildArray); wildCounts = 0; maxWEntry = 5;
            wildArray = NULL;
            free(expand);
            if(breakF) break;

        } else if (ch==27) {
               
            // Escape sequence. Read two chars more
            //
            // HINT: Use the program "keyboard-example" to
            // see the ascii code for the different chars typed.
            //
            //OR...have a window open in safari of the ASCII table...
            
            // Read next two characters
            char ch1; 
            char ch2;
            read(0, &ch1, 1);
            read(0, &ch2, 1);

            if (ch1==91 && ch2==65) {
                
                // Up arrow. Print next line in history.
                
                // Check if any history
                if(!history_length) continue;
                
                // Erase old line
                // Print backspaces
                int i = 0;
                for (; i++ < line_length;) {
                    
                    ch = 8;
                    write(1,&ch,1);

                } i = 0;

                // Print spaces on top
                for (; i++ < line_length;) {
                    
                    ch = ' ';
                    write(1,&ch,1);

                } i = 0;

                // Print backspaces
                for (; i++ < line_length;) {
                    
                    ch = 8;
                    write(1,&ch,1);

                }	

                // Change index accoringly and copy buffer
                if (history_index<history_length) history_index++;
                strcpy(line_buffer, history[history_length-history_index]);
                line_length=strlen(line_buffer);

                // Write buffer and set index.
                write(1, line_buffer, line_length);
                index = line_length;

            } else if (ch1 == 91 && ch2 == 67) {

                // RIGHT ARROW.

                // Chack if index at end of line
                if(index==line_length) continue;

                // Simulate right arrow
                ch=27;
                write(1,&ch,1);
                ch=91;
                write(1,&ch,1);
                ch=67;
                write(1,&ch,1);
                index++;

            } else if (ch1 == 91 && ch2 == 68) {

                // LEFT ARROW.

                // Check if we do anything first.
                if (!index) continue;

                // Simulate left arrow
                ch = 27;
                write(1,&ch,1);
                ch=91;
                write(1,&ch,1);
                ch=68;
                write(1,&ch,1);
                index--;

            } else if (ch1 == 91 && ch2 == 66) {

                // DOWN ARROW.

                // Erase line.
                int i = 0;
                for (; i++ < line_length;) {

                    ch = 8;
                    write(1, &ch, 1);

                } i = 0;

                // Print spaces.
                for (; i++ < line_length;) {

                    ch = ' ';
                    write(1, &ch, 1);

                } i = 0;

                // Print backspace.
                for (; i++ < line_length;) {

                    ch = 8;
                    write(1, &ch, 1);

                }

                // Adjust history accordingly and copy buffer
                if(history_index-1 >0) {history_index--; strcpy(line_buffer,history[history_length-history_index]);}
                else strcpy(line_buffer,"");
                if(history_index==1) history_index=0; //FINALLY FIXED IT
                line_length=strlen(line_buffer);

                // Print history to buffer
                write(1,line_buffer,line_length);
                index=line_length;

            // Quick-fix for Delete - FIX LATER    
            } else if (ch1==91 && ch2==51) { goto ctrlD; }
        
        }            

    }

    // Append enter and null character
    line_buffer[line_length]=10;
    line_length++;
    line_buffer[line_length]=0;

    // Restore attributes and return
    tcsetattr(0,TCSANOW,&saveTemp);
    return line_buffer;
}


void expandTab(char* prefix, char* suffix) {

    // Perform Wildcard expansion.
    //printf("%s\n", prefix);
    //if(*suffix=='/') *prefix = '/';
    if (wildArray == NULL) {
        
        // Initialize array.
        wildArray = (char**)malloc(maxWEntry*sizeof(char*));

    }

    if (!*suffix) {

        // Suffix is empty - insert prefix!
        
        
        // Resize the mildArray if needed.
        if (maxWEntry == wildCounts) {
        
            // Just realloc with new maxWEntries - don't copy - realloc retains memory.
            maxWEntry <<= 1;
            wildArray = (char**)realloc(wildArray, maxWEntry*sizeof(char*));
        
        }

        // Insert prefix now.
        if(prefix[strlen(prefix)-1]=='/') prefix[strlen(prefix)-1] = 0;
        wildArray[wildCounts] = strdup(prefix);
        wildCounts++;
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
        if(*component) expandTab(newPrefix, suffix);
        else expandTab("", suffix);

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
                if(*component == '.') expandTab(newPrefix, suffix);
            }//else if (*component != '.') continue;
            else expandTab(newPrefix, suffix);

        }

    }

    //free(rgx); free(component); free(newPrefix);
    //delete regex;
    return;

}
