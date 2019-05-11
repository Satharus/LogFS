#ifndef SHELLFUNCTIONS_H
#define SHELLFUNCTIONS_H

char* getShellCommandOutput(char command[]);
void writeToFile(char filename[], char* content, char mode[]);
void removeLogDuplicates(char * filepath);

#endif // SHELLFUNCTIONS_H
