/* 
 * File:   utils.c
 * Author: Bartjan Zondag & Kevin Rosendaal
 *
 */

/*
 * Functies die in zowel de client- als serverapplicatie gelijk zijn.
 */

#include "pfc.h"

/*Global vars*/
int bestandfd;

/*
 * Functie open bestand
 */
int OpenBestand(char* bestandsnaam){
    if((bestandfd = open(bestandsnaam, O_RDONLY)) < 0){
        return -1;
    }
    return 0;
}

/*
 * Functie controleerd of bestand bestaat
 */
int BestaatDeFile(char* fileName){
    if(access(fileName, F_OK) < 0) {
        // Bestand bestaat niet op schijf van client
        return -1;
    } else {
        // Bestand bestaat op schijf van client
        return 0;
    }
}

/*
 * Functie geef aan dat je een bestand wilt uploaden
 */
int FileTransferSend(int* sockfd, char* bestandsnaam){
    
    char buffer[BUFFERSIZE], statusCode[4];
    int readCounter = 0;
    
    sprintf(statusCode, "%d", STATUS_CR);
    strcpy(buffer, statusCode); // status-code acceptatie en naam van bestand
    strcat(buffer, ":");
    strcat(buffer, bestandsnaam);
    if((send(*sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Send metadata error:");
        return -1;
    }
    if((recv(*sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Receive metadata OK error:");
        return -1;
    }
    
    printf("%s\n", buffer);

    if(switchResult(sockfd, buffer) != STATUS_OK){return -1;}
        
    printf("Meta data succesvol verstuurd en ok ontvangen\n");

    /*
     * Lees gegevens uit een bestand. Zet deze in de buffer. Stuur buffer naar server.
     * Herhaal tot bestand compleet ingelezen is.
     */
    while((readCounter = read(bestandfd, &buffer, BUFFERSIZE)) > 0){
        if((send(*sockfd, buffer, readCounter, 0)) < 0) {
            perror("Send file error:");
            return -1;
        }
        
        printf("pakket verstuurd! %i\n", readCounter);

        if((recv(*sockfd, buffer, readCounter, 0)) < 0) {
            perror("Receive metadata OK error:");
            return -1;
        }
        
        if(switchResult(sockfd, buffer) != STATUS_OK){return -1;}
    }

    if(readCounter < 0){
        return -1;
    }

    /*
     * Bestand klaar met versturen. Geef dit aan aan server doormiddel van 101:EOF
     */
    
    sprintf(statusCode, "%d", STATUS_EOF);
    
    strcpy(buffer,statusCode);

    if((send(*sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Send 101 error:");
        return -1;
    }

    if((recv(*sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Receive OK error:");
        return -1;
    }
    
    if(switchResult(sockfd, buffer) != STATUS_OK){return -1;}

    printf("EOF verstuurd en ok ontvangen. verbinding wordt verbroken\n");

    close(*sockfd);
    close(bestandfd);
    
    return 0;
}

/*
 * Functie verstuurt de modify-date van een bestand naar de server en
 * krijgt terug welke de nieuwste is.
 */
int ModifyCheckClient(int* sockfd, char* bestandsnaam){
    
    struct stat bestandEigenschappen;
    stat(bestandsnaam, &bestandEigenschappen);
    
    char statusCode[3], seconden[40];
    char* buffer = malloc(BUFFERSIZE);
    //int readCounter = 0;
    
    sprintf(seconden, "%i", (int) bestandEigenschappen.st_mtime);
    sprintf(statusCode, "%d", STATUS_MODCHK);
    
    strcpy(buffer, statusCode); // status-code acceptatie en naam van bestand
    strcat(buffer, ":");
    strcat(buffer, bestandsnaam);
    strcat(buffer, ":");
    strcat(buffer, seconden);
    
    // 301:naambestand:seconden - Request
    if((send(*sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Send modifycheck request error:");
        return -1;
    }
    
    // Wacht op antwoord modifycheck van server
    if((recv(*sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Receive modififycheck result error:");
        return -1;
    }
    
    switchResult(sockfd, buffer);
    free(buffer);
    return 0;
}