/* 
 * File:   utils.c
 * Author: Bartjan Zondag & Kevin Rosendaal & Brord van Wierst
 *
 * Deze file bevat code die door zowel de client als server gebruikt kan worden.
 */

/*
 * Functies die in zowel de client- als serverapplicatie gelijk zijn.
 */

#include "pfc.h"

/*Global vars*/
int bestandfd; //moet nog worden verwerkt in functies zodat dit niet nodig is

/**
 * Functie OpenBestand
 * Hier wordt een bestand geopend.
 * 
 * @param bestandsnaam bestandsnaam van bestand dat gecontroleerd moet worden
 * @return 0 if succesvol. -1 if failed.
 */
int OpenBestand(char* bestandsnaam){
    if((bestandfd = open(bestandsnaam, O_RDONLY)) < 0){
        return STUK;
    }
    return MOOI;
}

/*
 * Functie controleerd of bestand bestaat
 * @param bestandsnaam bestandsnaam van bestand dat gecontroleerd moet worden
 * @return 0 if succesvol. -1 if failed.
 */
int BestaatDeFile(char* bestandsnaam){
    if(access(bestandsnaam, F_OK) < 0) {
        // Bestand bestaat niet op schijf van client
        return STUK;
    } else {
        // Bestand bestaat op schijf van client
        return MOOI;
    }
}

/*
 * Functie geef aan dat je een bestand wilt uploaden
 * @param sockfd de socket waar geluisterd/verzonden wordt
 * @param bestandsnaam bestandsnaam van bestand dat gecontroleerd moet worden
 * @return 0 if succesvol. -1 if failed.
 */
int FileTransferSend(int* sockfd, char* bestandsnaam){
    char* savedir = malloc(strlen(bestandsnaam));
    if (IS_CLIENT == STUK){
        if (realloc(savedir, strlen("userfolders/") + strlen(bestandsnaam) + strlen(clients[*sockfd-4].username)) == NULL) return STUK;
        strcpy(savedir, "userfolders/");
        strcat(savedir, clients[*sockfd-4].username);
        strcat(savedir, "/");
    }
    strcat(savedir, bestandsnaam);
    
    char buffer[BUFFERSIZE];
    int readCounter = 0;
    
    if (waitForOk(*sockfd) == MOOI){
        printf("Received ok!");
    } else {
        printf("error?\n");
        return STUK;
    }
    
    if((OpenBestand(savedir)) < 0){return -1;}
    puts(buffer);
    
    /*
     * Lees gegevens uit een bestand. Zet deze in de buffer. Stuur buffer naar server.
     * Herhaal tot bestand compleet ingelezen is.
     */
    while((readCounter = read(bestandfd, buffer, BUFFERSIZE)) > 0){
        if((send(*sockfd, buffer, readCounter, 0)) < 0) {
            perror("Send file error");
            return STUK;
        }
        
        if (waitForOk(*sockfd) == MOOI){
            //printf("Received ok!");
        } else {
            printf("error?");
            break;
        }
        
        //bzero(buffer, BUFFERSIZE);
    }
    if(readCounter < 0){
        return STUK;
    }
    
    /*
     * Bestand klaar met versturen. Geef dit aan aan server doormiddel van 101:EOF
     */
    if (sendPacket(*sockfd, STATUS_EOF, NULL) == STUK){
        return STUK;
    }
    
    if (waitForOk(*sockfd) == STUK){
        //??
    } else {
        printf("EOF verstuurd en ok ontvangen. \n");
    }
    
    close(bestandfd);
    return MOOI;
}

/**
 * Functie om bestanden te ontvangen
 * @param sockfd socket waarop geluisterd/verzonden wordt
 * @param bestandsnaam bestandsnaam van bestand dat gecontroleerd moet worden
 * @param time de tijd die het nieuwe bestand moet krijgen als modify datum
 * @return 0 if succesvol. -1 if failed.
 */
int FileTransferReceive(int* sockfd, char* bestandsnaam, int time){
    char* savedir = malloc(strlen(bestandsnaam));
    if (IS_CLIENT == STUK){
        if (realloc(savedir, strlen("/userfolders/") + strlen(bestandsnaam) + strlen(clients[*sockfd-4].username)) == NULL) return STUK;
        strcpy(savedir, "userfolders/");
        strcat(savedir, clients[*sockfd-4].username);
    }
    strcat(savedir, bestandsnaam);
    
    char buffer[BUFFERSIZE];
    int file = -1, rec = 0;
    
    file = open(savedir, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file < 0){
        //file doesnt exist, lets create the folder shant we?
        char **path = malloc(strlen(savedir));
        puts(savedir);
        int i, amount = transformWith(savedir, path, "/");
        if (amount > 0){
            char *folderpath = malloc(strlen(savedir) + strlen("userfolders/") + 100);
            strcpy(folderpath, path[0]);
            
            for(i=1; i<amount-1; i++){
                mkdir(folderpath, S_IRWXU);
                printf("mkdir'd: %s, added %s\n", folderpath, path[i]);
                strcat(folderpath, "/");
                strcat(folderpath, path[i]);
            }
        }
        
        file = open(savedir, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
    sendPacket(*sockfd, STATUS_OK, NULL);
    
    printf("TIJD: %i\n", time);
    
    for ( ;; ){
        if ((rec = recv(*sockfd, buffer, BUFFERSIZE,0)) < 0){
            return STUK;
        } else if (rec == 0){
            return MOOI;
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
            write(file, buffer, rec);

            //clear received data
            bzero(buffer, BUFFERSIZE);
        }
    }
    changeModTime(savedir, time);
    
    
    
    close(file);
    return MOOI;
}

/**
 * Sends a packet to the fd defined
 * @param fd The fd to send this packet to
 * @param packet the packet number
 * @param ... the values for this packet, end with NULL
 * @return 1 if the packet was send succesfully. otherwise 0
 */

/**
 * Functie voor de server om te kijken of het bestand op de server nieuwer is of op de client
 * @param sockfd socket waarop gecontroleerd moet worden
 * @param bestandsnaam bestandsnaam van bestand dat gecontroleerd moet worden
 * @param timeleft de modify datum van het bestand van de client
 * @return 0 if succesvol. -1 if failed. 
 */
int ModifyCheckServer(int* sockfd, char *bestandsnaam, char* timeleft){
    int file, time = atoi(timeleft);
    printf("timeleft: %s, time: %i\n", timeleft, time);
    
    char *buffer = malloc((sizeof(int)*2)+(sizeof(char)*2)+strlen(bestandsnaam));
    char *savedir = malloc(strlen(bestandsnaam));
    
    if (IS_CLIENT == STUK){
        if (realloc(savedir, strlen("/userfolders/") + strlen(bestandsnaam) + strlen(clients[*sockfd-4].username)) == NULL) return STUK;
        strcpy(savedir, "userfolders/");
        strcat(savedir, clients[*sockfd-4].username);
        strcat(savedir, "/");
    }
    
    strcat(savedir, bestandsnaam);
    
    if ((file = open(savedir, O_RDONLY, 0666)) < 0){
        sendPacket(*sockfd, STATUS_OLD, bestandsnaam, NULL);
        
        sprintf(buffer, "%i", STATUS_NEW);
        strcat(buffer, ":");
        strcat(buffer, bestandsnaam);
        strcat(buffer, ":");
        strcat(buffer, toString(time));
    } else {
        int owntime;
        if ((owntime = modifiedTime(savedir)) < time){
            //own file older
            sendPacket(*sockfd, STATUS_OLD, bestandsnaam, NULL);
            //sendPacket(*sockfd, STATUS_NEW, NULL);

            sprintf(buffer, "%i", STATUS_NEW);
            strcat(buffer, ":");
            strcat(buffer, bestandsnaam);
            strcat(buffer, ":");
            strcat(buffer, toString(time));
        } else if (owntime > time){
            //own file newer  
            sendPacket(*sockfd, STATUS_NEW, bestandsnaam, toString(owntime), NULL);

            sprintf(buffer, "%i", STATUS_OLD);
            strcat(buffer, ":");
            strcat(buffer, bestandsnaam);
        } else {
            //same
            sendPacket(*sockfd, STATUS_SAME, NULL);
            return MOOI;
        }
    }
    close(file);
    if (waitForOk(*sockfd) == MOOI){
        return switchResult(sockfd, buffer);
    } else {
        return STUK;
    }
    
}

int sendPacket(int fd, int packet, ...){
    char *info = malloc(105);
    strcpy(info, "");

    char *p = malloc(105);
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
    printf("Send packet: %i, data: \"%s\"(bytes: %i)\n", packet, info, strlen(info));
    return MOOI;
}

/**
 * Functie om nummer om te zetten naar string
 * @param number integer nummer
 * @return de meegegeven integer als string
 */
char *toString(int number){
    char *nr = malloc(10);
    sprintf(nr, "%i", number);
    strcat(nr, "\0");
    return nr;
}

/**
 * Creates the text needed for EOF packet
 * @param to where we place the text in
 */
void getEOF(char *to){
    strcpy(to, "");
    char *pID = malloc(sizeof(int));
    sprintf(pID, "%i", STATUS_EOF);
    strcat(to, pID);
    strcat(to, ":EOF");
}

/**
 * returned de tijd van een file vanaf 1970
 * @param bestandsnaam de bestandsnaam waar de tijd van gecontroleerd moet worden
 * @return sec vanaf 1970 of -1
 */
int modifiedTime(char *bestandsnaam){
    struct stat eigenschappen;
    
    if (stat(bestandsnaam, &eigenschappen) < 0){
        return STUK;
    }
    
    return eigenschappen.st_mtim.tv_sec;
}
/**
 * Functie die wacht op een OK , of ander pakket, en terug geeft of dit een ok is of niet
 * @param sockfd de socket waarop de functie een ok verwacht
 * @return 0 if succesvol. -1 if failed.
 */
int waitForOk(int sockfd){
    char *buffer = malloc(BUFFERSIZE);
    if((recv(sockfd, buffer, BUFFERSIZE, 0)) < 0) {
        perror("Receive OK error");
        return STUK;
    }
    
    if(switchResult(&sockfd, buffer) != STATUS_OK){return STUK;}
    return MOOI;
}
/**
 * Functie veranderd de modify/access datum van een bestand
 * @param bestandsnaam naam van het bestand dat aangepast moet worden
 * @param tijd de tijd die de modify/access datum moet worden van het bestand
 * @return 0 if succesvol. -1 if failed.
 */
int changeModTime(char *bestandsnaam, int time){
    struct timeval t[2];
    t[0].tv_sec = time;
    t[0].tv_usec = 0;
    
    t[1].tv_sec = time;
    t[1].tv_usec = 0;
    
    return utimes(bestandsnaam, t);
}

/**
 * Functie hashed wachtwoorden voordat deze de database ingaan.
 * @return 0 if succesvol
 */
int hashPassword(char *password, char *salt, char to[]) {
    SHA256_CTX ctx;
    unsigned char *temp = malloc(SHA256_DIGEST_LENGTH);
    
    // Initialiseer struct
    SHA256_Init(&ctx);
    
    // Plaats wachtwoord in struct 
    SHA256_Update(&ctx, password,  strlen(password));

    // Voeg salt en wachtwoord samen
    SHA256_Update(&ctx, salt, strlen(salt)); 
    
    // Genereer de hash
    SHA256_Final(temp, &ctx);
    
    // Maak van Hex een String
    convertHashToString(to, temp);
    
    return MOOI;
}

/**
 * Functie wordt gebruikt om een willekeurig salt te genereren
 * @param salt - pointer naar een saltvar in de functie hashPassword()
 * @param aantalBytes - Aantal bytes voor het salt
 * @return 
 */
int randomSalt(char *salt, int aantalBytes) {
    int i;
    srand(time(NULL));
    
    for(i = 0; i < aantalBytes; i++){
        salt[i] = (rand() % 26) + 97;
    }
    
    return MOOI;
}

/**
 * Functie om hexadecimale hashes te converteren naar strings
 * @param stringHash - Pointer naar nieuwe string
 * @param hash - Hexadecimale unsigned char[]
 * @return 
 */
int convertHashToString(char *stringHash, unsigned char hash[]) {
    char temp[3];
    int i;
    
    bzero(stringHash, SHA256_DIGEST_LENGTH);
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(temp, "%02x", hash[i]);
        strcat(stringHash, temp);
    }
    return MOOI;
}

/**
 * Functie om een array te printen
 * @param length de lengte van de array
 * @param array de array met strings
 */
void printArray(int length, char *array[]){
    int i;
    for (i=0; i<length; i++){
        printf("%s, ", array[i]);
    }
    puts("\n");
}

/**
 * Functie om input te ontvangen
 * @param max het maximaal aantal chars dat mag worden meegegeven via fgets
 * @return return van de char array
 */
char* getInput(int max){
    char *temp = malloc(max);
    fgets(temp,max,stdin);
    temp[strlen(temp)-1]='\0';
    return temp;
}

/**
 * deze functie print het begin scherm
 */
void printStart(void){
    system("clear");
    puts("-----------------------------");
    puts("|    Welcome to sorbet!     |");
    puts("|             . ,           |");
    puts("|          *    ,           |");
    puts("|     ` *~.|,~* '           |");
    puts("|     '  ,~*~~* `     _     |");
    puts("|      ,* / \\`* '    //     |");
    puts("|       ,* ; \\,O.   //      |");
    puts("|           ,(:::)=//       |");
    puts("|         (  `~(###)        |");
    puts("|           %---'`\"y        |");
    puts("|            \\    /         |");
    puts("|             \\  /          |");
    puts("|            __)(__         |");
    puts("|           '------`        |");
    puts("-----------------------------\n");
}
