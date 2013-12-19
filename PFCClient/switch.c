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

int switchResult(SSL* ssl, char* buffer){
    printf("raw packet data: [%s], length: %i\n", buffer, strlen(buffer));
    char** to = malloc(strlen(buffer));
    int bytes = strlen(buffer), ret = 0;
    int statusCode = 0, aantal = transform(buffer, to);
    if(to[0] == NULL){printf("to[0] is NULL\n"); return -1;}
    
    if((statusCode = atoi(to[0])) < 100){
        printf("error with statusCode (%i)\n", statusCode);
        return STUK;
    }
    printf("Received packet: %i(%i) data: \n", statusCode, bytes);
    printArray(aantal, to);
    bzero(buffer, strlen(buffer));
    
    switch(statusCode) {
        case STATUS_OK:       ret = STATUS_OK; break;
        case STATUS_EOF:      ret = STATUS_EOF; break;
        case STATUS_SAME:     ret = MOOI; break;
        case STATUS_AUTHOK:   ret = STATUS_AUTHOK; break;
        case STATUS_AUTHFAIL: ret = STATUS_AUTHFAIL; break;
        
        case STATUS_MKDIR:    ret = CreateFolder(ssl, to[1]); break;
        case STATUS_CR:       ret = FileTransferReceive(ssl, to[1], atoi(to[2])); break;
        case STATUS_MODCHK:   ret = ModifyCheckServer(ssl, to[1], to[2]); break; //server
        case STATUS_OLD:      ret = FileTransferSend(ssl, to[1]); break;
        case STATUS_NEW:      ret = FileTransferReceive(ssl, to[1], atoi(to[2])); break;
        case STATUS_CNA:      ret = ConnectRefused(ssl); break;
        default:              ret = STUK;
    }
    
    if(to){
        puts("hodor to in switch");
        //free(*to);
        to = NULL;
    }
    
    buffer = NULL;
    
    return ret;
}

int ConnectRefused(SSL* ssl){
    printf("You have been disconnected\n");
    
    close(SSL_get_fd(ssl));
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
    char *temp = malloc(strlen(text));
    bzero(temp, strlen(text));
    
    temp = strtok(text, delimit);
    int numVars = 0;

    while (temp != NULL || temp != '\0'){
        to[numVars] = malloc(strlen(temp));
        bzero(to[numVars], strlen(temp));
        
        to[numVars] = temp;
        
        //bzero(temp, strlen(text));
        temp = strtok(NULL, delimit);
        numVars++;
    }
    
    free(temp);
    return numVars;
}
