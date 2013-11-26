/* 
 * File:   switch.c
 * Author: Bartjan Zondag & Kevin Rosendaal
 *
 */

/*
 * Switch voor het maken van een structuur voor status-codes
 */

#include "pfcclient.h"

int switchResult(char* buffer){
    
    char** to = malloc(1);
    int statusCode = 0;
    
    transform(buffer, to);
    
    if((statusCode = atoi(to[0])) < 100){
        printf("error with statusCode\n");
        return -1;
    }
    
    switch(statusCode) {
        case STATUS_OK:      return STATUS_OK;
        case STATUS_EOF:     return STATUS_EOF;
        case STATUS_CR:      return FileTransfer(); 
        case STATUS_MODCHK:  return ModifyCheck();
    }
    
}

/**
 * Turn a string into an array of strings split by the identifier ":"
 * @param text the text to split
 * @param to a pointer to the array
 * @return the amount of splits done (amount of values)
 */
int transform(char *text, char** to){
    char *temp;

    temp = strtok(text, ":");
    int numVars = 0;

    while (temp != NULL || temp != '\0'){
        to[numVars] = temp;
        temp = strtok(NULL, ":");
        numVars++;
    }
    
    return numVars;
}