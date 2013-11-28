/* 
 * File:   utils.c
 * Author: Bartjan Zondag & Kevin Rosendaal & Brord van Wierst
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
    
//    sprintf(statusCode, "%d", STATUS_CR);
//    strcpy(buffer, statusCode); // status-code acceptatie en naam van bestand
//    strcat(buffer, ":");
//    strcat(buffer, bestandsnaam);
//    if((send(*sockfd, buffer, strlen(buffer), 0)) < 0) {
//        perror("Send metadata error");
//        return -1;
//    }
    
    if((recv(*sockfd, buffer, BUFFERSIZE, 0)) < 0) {
        perror("Receive metadata OK error");
        return -1;
    }
    
    if((OpenBestand(bestandsnaam)) < 0){return -1;}
    
    
    printf("%s\n", buffer);

    if(switchResult(sockfd, buffer) != STATUS_OK){return -1;}
        
    printf("ok ontvangen\n");

    /*
     * Lees gegevens uit een bestand. Zet deze in de buffer. Stuur buffer naar server.
     * Herhaal tot bestand compleet ingelezen is.
     */
    while((readCounter = read(bestandfd, buffer, BUFFERSIZE)) > 0){
        printf("readCounter: %i\n",readCounter);
        if((send(*sockfd, buffer, readCounter, 0)) < 0) {
            perror("Send file error");
            return -1;
        }
        
        printf("pakket verstuurd! %i(%s)\n", readCounter, buffer);
        
        if((recv(*sockfd, buffer, readCounter, 0)) < 0) {
            perror("Receive metadata OK error");
            return -1;
        }
        
        if(switchResult(sockfd, buffer) != STATUS_OK){return -1;}
    }
    printf("dude not cool\n");
    if(readCounter < 0){
        return -1;
    }

    /*
     * Bestand klaar met versturen. Geef dit aan aan server doormiddel van 101:EOF
     */
    
    sprintf(statusCode, "%d", STATUS_EOF);
    
    strcpy(buffer,statusCode);

    if((send(*sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Send 101 error");
        return -1;
    }

    if((recv(*sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Receive OK error");
        return -1;
    }
    
    if(switchResult(sockfd, buffer) != STATUS_OK){return -1;}

    printf("EOF verstuurd en ok ontvangen. verbinding wordt verbroken\n");

    close(*sockfd);
    close(bestandfd);
    
    return 0;
}

int FileTransferReceive(int* sockfd, char* bestandsnaam){
    char buffer[BUFFERSIZE];
    int file = -1, rec = 0;;
    file = open(bestandsnaam, O_WRONLY | O_CREAT, 0666);
    if (file < 0){
        perror("open");
        return -1;
    }
    sendPacket(*sockfd, STATUS_OK, NULL);
    
    for ( ;; ){
        if ((rec = recv(*sockfd, buffer, sizeof(buffer),0)) < 0){
            return -1;
        } else if (rec == 0){
            return 0;
        } else {
            if (rec < 50){
                //want to stop?
                if(switchResult(sockfd, buffer) == STATUS_EOF){
                    printf("Stopping file transfer!\n");
                    sendPacket(*sockfd, STATUS_OK, NULL);
                    break;
                }
            }
            
            //ack that we received data
            sendPacket(*sockfd, STATUS_OK, toString(rec), NULL);
            //save it all
            printf("Received %i bytes of data\n", rec);
            write(file, buffer, rec);

            //clear received data
            memset(buffer, 0, sizeof(buffer));
        }
    }
    close(file);
    return 1;
}

/**
 * 
 * @param sockfd
 * @param bestandsnaam
 * @param timeleft
 * @return 0 if succesvol. -1 if failed. 
 */
int ModifyCheckServer(int* sockfd, char *bestandsnaam, char* timeleft){
    int file, time = atoi(timeleft);
    char *buffer = malloc(1);
    
    if ((file = open(bestandsnaam, O_RDONLY, 0666)) < 0){
        sendPacket(*sockfd, STATUS_OLD, NULL);
        
        sprintf(buffer, "%i", STATUS_CR);
        strcat(buffer, ":");
        strcat(buffer, bestandsnaam);
    } else {
        int owntime;
        if ((owntime = modifiedTime(bestandsnaam)) < time){
            //old
            sendPacket(*sockfd, STATUS_OLD, NULL);
            //sendPacket(*sockfd, STATUS_NEW, NULL);

            sprintf(buffer, "%i", STATUS_NEW);
            strcat(buffer, ":");
            strcat(buffer, bestandsnaam);

        } else if (owntime > time){
            //newer  
            sendPacket(*sockfd, STATUS_NEW, NULL);

            sprintf(buffer, "%i", STATUS_OLD);
            strcat(buffer, ":");
            strcat(buffer, bestandsnaam);
        } else {
            //same
            printf("ze zijn gelijk !!\n");
            return MOOI;
        }
    }
    
    switchResult(sockfd, buffer);
    return MOOI;
}


/**
 * Functie verstuurt de modify-date van een bestand naar de server en
 * krijgt terug welke de nieuwste is.
 * @param sockfd
 * @param bestandsnaam 
 * @return 0 if succesvol. -1 if failed.
 */
int ModifyCheckClient(int* sockfd, char* bestandsnaam){
    
    struct stat bestandEigenschappen;
    stat(bestandsnaam, &bestandEigenschappen);
    
    char statusCode[4], seconden[40];
    char* buffer = malloc(BUFFERSIZE);
    int readCounter = 0;
    
    sprintf(seconden, "%i", (int) bestandEigenschappen.st_mtime);
    sprintf(statusCode, "%d", STATUS_MODCHK);
    
    sendPacket(*sockfd, STATUS_MODCHK, bestandsnaam, seconden, NULL);
    
    // Wacht op antwoord modifycheck van server
    if((readCounter = recv(*sockfd, buffer, BUFFERSIZE, 0)) <= 0) {
        printf("%s(%i)\n", buffer, readCounter);
        perror("Receive modififycheck result error1");
        return -1;
    }
    
    strcat(buffer,":");
    strcat(buffer,bestandsnaam);
    switchResult(sockfd, buffer);
    
    free(buffer);
    return 0;
}

/**
 * Sends a packet to the fd defined
 * @param fd The fd to send this packet to
 * @param packet the packet number
 * @param ... the values for this packet, end with NULL
 * @return 1 if the packet was send succesfully. otherwise 0
 */
int sendPacket(int fd, int packet, ...){

    char *info = malloc(20);
    strcpy(info, "");

    char *p = malloc(20);
    sprintf(p, "%i", packet);
    
    va_list ap;
    char *i;
    
    va_start(ap, packet);
    for (i = p; i != NULL; i = va_arg(ap, char *)){
        sprintf(p, "%s", i);
        strcat(info, p);
        strcat(info, ":");
    }
    va_end(ap);
    
    info[strlen(info)-1] = '\0';
    
    int bytes;
    
    if((bytes=send(fd, info, strlen(info),0)) < 0){
        perror("send");
        return STUK;
    }
    printf("packet: %i, info: %s(%i)\n", packet, info, bytes);
    return MOOI;
}

char *toString(int number){
    char *nr = malloc(10);
    sprintf(nr, "%i", number);
    return nr;
}

/**
 * Creates the text needed for EOF packet
 * @param to where we place the text in
 */
void getEOF(char *to){
    strcpy(to, "");
    char *pID = malloc(20);
    sprintf(pID, "%d", STATUS_EOF);
    strcat(to, pID);
    strcat(to, ":EOF");
}

/**
 * 
 * @param bestandsnaam 
 * @return sec vanaf 1970 of -1
 */
int modifiedTime(char *bestandsnaam){
    struct stat eigenschappen;
    
    if (stat(bestandsnaam, &eigenschappen) < 0){
        return STUK;
    }
    
    return eigenschappen.st_mtim.tv_sec;
}
