/* 
 * File:   switch.c
 * Author: Bartjan Zondag & Kevin Rosendaal
 *
 */

/**
 * Switch voor het maken van een structuur voor status-codes
 * @param sockfd socket waarop gecontroleerd moet worden
 * @param buffer data in string format
 * @return 0 if succesvol. -1 if failed.
 */

#include "pfc.h"

int switchResult(int* sockfd, char* buffer){
    char** to = malloc(1);
    int statusCode = 0;
    
    transform(buffer, to);
    
    if(to[0] == NULL){printf("to[0] is NULL\n"); return -1;}
    
    if((statusCode = atoi(to[0])) < 100){
        printf("error with statusCode\n");
        return -1;
    }
    printf("received packet: %i(%i)\n", statusCode, strlen(buffer));
    
    switch(statusCode) {
        case STATUS_OK:      return STATUS_OK;
        case STATUS_EOF:     return STATUS_EOF;
        
        case STATUS_CR:      return FileTransferReceive(sockfd, to[1], atoi(to[2]));
        case STATUS_MODCHK:  return ModifyCheckServer(sockfd, to[1], to[2]); //server
        case STATUS_OLD:     return FileTransferSend(sockfd, to[1]);
        case STATUS_NEW:     return FileTransferReceive(sockfd, to[1], atoi(to[2]));
        default:             return MOOI;
    }
}

/**
 * Turn a string into an array of strings split by the identifier ":"
 * @param text the text to split
 * @param to a pointer to the array
 * @return the amount of splits done (amount of values)
 */
int transform(char *text, char** to){
    char *lim = ":";
    return transformWith(text, to, lim);
}

/**
 * Turn a string into an array of strings split by the identifier ":"
 * @param text the text to split
 * @param to a pointer to the array
 * @return the amount of splits done (amount of values)
 */
int transformWith(char *text, char** to, char *delimit){
    char *temp;

    temp = strtok(text, delimit);
    int numVars = 0;

    while (temp != NULL || temp != '\0'){
        to[numVars] = temp;
        temp = strtok(NULL, ":");
        numVars++;
    }
    return numVars;
}
