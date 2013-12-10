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

int switchResult(SSL *ssl, char *buffer){
    char** to = malloc(sizeof(buffer)*sizeof(buffer[0]));
    int statusCode = 0, bytes = strlen(buffer), aantal = transform(buffer, to);
    
    if(to[0] == NULL){printf("to[0] is NULL\n"); return -1;}
    
    if((statusCode = atoi(to[0])) < 100){
        printf("error with statusCode\n");
        return STUK;
    }
    printf("Received packet: %i(%i) data: \n", statusCode, bytes);
    printArray(aantal, to);
    bzero(buffer, strlen(buffer));
    
    switch(statusCode) {
        case STATUS_OK:       return STATUS_OK;
        case STATUS_EOF:      return STATUS_EOF;
        case STATUS_SAME:     return MOOI;
        case STATUS_AUTHOK:   return STATUS_AUTHOK;
        case STATUS_AUTHFAIL: return STATUS_AUTHFAIL;
        
        case STATUS_CR:       return FileTransferReceive(ssl, to[1], atoi(to[2]));
        case STATUS_MODCHK:   return ModifyCheckServer(ssl, to[1], to[2]); //server
        case STATUS_OLD:      return FileTransferSend(ssl, to[1]);
        case STATUS_NEW:      return FileTransferReceive(ssl, to[1], atoi(to[2]));
        case STATUS_CNA:      return ConnectRefused(ssl);
        default:              return STUK;
    }
}

int ConnectRefused(SSL *ssl){
    printf("You have been disconnected\n");
    int fd;
    
    fd = SSL_get_fd(ssl);
    SSL_free(ssl);
    close(fd);
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
    char *temp;

    temp = strtok(text, delimit);
    int numVars = 0;

    while (temp != NULL || temp != '\0'){
        to[numVars] = temp;
        temp = strtok(NULL, delimit);
        numVars++;
    }
    return numVars;
}
