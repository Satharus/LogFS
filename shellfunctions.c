#include <bits/types/FILE.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shellfunctions.h"

char* getShellCommandOutput(char command[])
{
	//This functions sends a command to the system shell and returns the output

    FILE *file; // Pointer to get the output channel of the command
    char *buff; //Buffer to get the output
    buff = (char *) malloc(sizeof(char) *1024);
    
    file = popen(command, "r");
    
    char *output;
    output = (char *) malloc(sizeof(char) * 1024);
    memset(output, '\0', strlen(output)); //Reset the string

    while (fgets(buff, sizeof(buff)-1, file) != NULL)
        strcat(output, buff); //Keep getting from the bugger while it isn't null
        //And append it to the output string
        
    pclose(file);
    	free(buff);
    return output;
}

void writeToFile(char filename[], char* content, char mode[])
{
	//This function writes the given content to the file using the specified mode
    FILE *f = fopen(filename, mode);
    fprintf(f, "%s", content);
    fclose(f);
}

void removeLogDuplicates(char * filepath)
{
	//This function removes the duplicate lines from the Log.txt file

	char *command = malloc(sizeof(char)*1024);
	/*
	 * The full command is oldSortedFile=`uniq "Log.txt"`; echo "$oldSortedFile" > "Log.txt"
	 * strcat is used to add the filepath
	 */
	strcat(command, "oldSortedFile=`uniq \"");
	strcat(command, filepath);
	strcat(command, "\"`; echo \"$oldSortedFile\" > \"");
	strcat(command, filepath);
	strcat(command, "\"");
	system(command);
	free(command);
}