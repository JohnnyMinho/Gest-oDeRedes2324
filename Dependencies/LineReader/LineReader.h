#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef LINEREADER_H
#define LINEREADER_H

extern int HowManyLines(int fd);

extern int* LinePosition(int fd, char *arr);

extern int HowManyCharsLine(char *buff);

#endif