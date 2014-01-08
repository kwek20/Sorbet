/* 
 * File:   switch.c
 * Author: Bartjan Zondag & Kevin Rosendaal & Brord van Wierst
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
    if(DEBUG >= 2){
        printf("[switchResult]: Raw packet data: [%s], length: %i\n", buffer, strlen(buffer));
    }
    char** to = malloc(10*sizeof(int));
    int ret = 0;
    int statusCode = 0, aantal = transform(buffer, to);
    if(to[0] == NULL){printf("to[0] is NULL\n"); return -1;}
    
    if((statusCode = atoi(to[0])) < 100){
        return STUK;
    }
    if(DEBUG >= 2){printArray(aantal, to);};
    bzero(buffer, strlen(buffer));
    
    switch(statusCode) {
        case STATUS_OK:       ret = STATUS_OK; break;
        case STATUS_EOF:      ret = STATUS_EOF; break; 
        case STATUS_SAME:     ret = MOOI; break;
        case STATUS_AUTHOK:   ret = STATUS_AUTHOK; break;
        case STATUS_AUTHFAIL: ret = STATUS_AUTHFAIL; break;
        
        case STATUS_MKDIR:    ret = CreateFolder(sockfd, to[1]); break;
        case STATUS_CR:       ret = FileTransferReceive(sockfd, to[1], atoi(to[2])); break;
        case STATUS_MODCHK:   ret = ModifyCheckServer(sockfd, to[1], to[2]); break; //server
        case STATUS_OLD:      ret = FileTransferSend(sockfd, to[1]); break;
        case STATUS_NEW:      ret = FileTransferReceive(sockfd, to[1], atoi(to[2])); break;
        case STATUS_CNA:      ret = ConnectRefused(sockfd); break;
        case STATUS_SYNC:     ret = loopOverFiles(sockfd, to[1]);
        default:              ret = STUK;
    }
    
    if(to){
        to = NULL;
    }
    
    return ret;
}

int ConnectRefused(int* sockfd){
    printf("You have been disconnected\n");
    close(*sockfd);
    return STUK;
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
    char *temp = malloc(BUFFERSIZE);
    bzero(temp, BUFFERSIZE);
    
    temp = strtok(text, delimit);
    
    int numVars = 0;

    while (temp != NULL || temp != '\0'){
        to[numVars] = malloc(strlen(temp)+1);
        bzero(to[numVars], strlen(temp)+1);
        strcpy(to[numVars], temp);
        temp = strtok(NULL, delimit);
        numVars++;
    }
    
    free(temp);
    return numVars;
}
