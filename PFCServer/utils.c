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
    char* savedir = fixServerBestand(sockfd, bestandsnaam);
    char *buffer = malloc(BUFFERSIZE);
    bzero(buffer, BUFFERSIZE);
    int readCounter = 0;
    if (waitForOk(*sockfd) != MOOI){
        printf("FileTransferSend: WaitForOK != Mooi\n");
        return STUK;
    }
    
    if((OpenBestand(savedir)) < 0){
        printf("OpenBestand(savedir) error: %s\n", savedir);
        return STUK;
    }
    
    /*
     * Lees gegevens uit een bestand. Zet deze in de buffer. Stuur buffer naar server.
     * Herhaal tot bestand compleet ingelezen is.
     */
    while((readCounter = read(bestandfd, buffer, BUFFERSIZE)) > 0){
        if((send(*sockfd, &readCounter, sizeof(readCounter), 0)) < 0) {
            perror("FileTransferSend (While): Send readCounter error");
            return STUK;
        }        
        
        if (IS_CLIENT == MOOI){
            char* tempbuf = malloc(readCounter+1);
            strcpy(tempbuf, buffer);
                
            bzero(buffer, BUFFERSIZE);
            buffer = aes_encrypt((unsigned char *)tempbuf, &readCounter);
            free(tempbuf);
        }
        
        if((send(*sockfd, buffer, readCounter, 0)) < 0) {
            perror("FileTransferSend (While): Send file error");
            return STUK;
        }
        
        if (waitForOk(*sockfd) == STUK){
            printf("FileTransferSend (While): WaitForOK != Mooi\n");
            break;
        }
        
        bzero(buffer, BUFFERSIZE);
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
    if (waitForOk(*sockfd) == MOOI){
        printf("EOF verstuurd en ok ontvangen.\n");
    }
    if(buffer){
        free(buffer);
        buffer = NULL;
    }
    
    if(savedir){;
        savedir = NULL;
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
    char* savedir = fixServerBestand(sockfd, bestandsnaam);
    char* filePath = malloc(sizeof(savedir) + 200);
    char *buffer = malloc(BUFFERSIZE);
    bzero(buffer, BUFFERSIZE);
    int file = -1, rec = 0;
    int recvCounter = 0;
    
    strcpy(filePath, savedir);
    
    file = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file < 0){
        //file doesnt exist, lets create the folder shant we?
        char **path = malloc(sizeof(int)*25);
        bzero(path, strlen(savedir));
        puts(savedir);
        int i, amount = transformWith(savedir, path, "/");
        if (amount > 0){
            char *folderpath = malloc(strlen(savedir) + 200);
            bzero(folderpath, strlen(savedir));
            strcpy(folderpath, "");
            for(i=0; i<amount-1; i++){
                if (strcmp(path[i], "..") == 0) continue;
                strcat(folderpath, path[i]);
                strcat(folderpath, "/");
                mkdir(folderpath, S_IRWXU);
            }
            
            folderpath = NULL;
            if (path){
                free(*path);
                *path = NULL;
            }
        }
        file = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (file < 0){
            printf("FileTransferReceive: open(filePath) error\n");
            return STUK;
        }
    }
    sendPacket(*sockfd, STATUS_OK, NULL);
    for ( ;; ){
        if (recvCounter == 0) {
            if ((recv(*sockfd, &recvCounter, sizeof(recvCounter), 0)) < 0){
                return STUK;
            }
            printf("recvCounter: %i\n", recvCounter);
                if(switchResult(sockfd, (char*)&recvCounter) == STATUS_EOF){
                    //printf("Stopping file transfer!\n");
                    sendPacket(*sockfd, STATUS_OK, NULL);
                    break;
                }
            
        }
        if ((rec = recv(*sockfd, buffer, BUFFERSIZE,0)) < 0){
            return STUK;
        } else if (rec == 0){
            return MOOI;
        } else {
//            if (rec < 50){
//                puts("6");
//                //want to stop?
//                if(switchResult(sockfd, buffer) == STATUS_EOF){
//                    //printf("Stopping file transfer!\n");
//                    puts("7");
//                    sendPacket(*sockfd, STATUS_OK, NULL);
//                    break;
//                }
//            }
            
            recvCounter -= rec;
            if (recvCounter <= 0) {
                sendPacket(*sockfd, STATUS_OK, toString(rec), NULL);
                recvCounter = 0;
            }
            //ack that we received data
            //sendPacket(*sockfd, STATUS_OK, toString(rec), NULL);
            //save it all
            if (IS_CLIENT == MOOI){
                 char* tempbuf = ( char *)malloc(rec);
                strcpy(tempbuf, buffer);
                
                buffer = aes_decrypt((unsigned char *)tempbuf, &rec);
                free(tempbuf);
            }
            
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
    printf("total: %i, best: %i\n", (int)((sizeof(int)*2)+(sizeof(char)*2)+strlen(bestandsnaam)), (int)strlen(bestandsnaam));
    int size = (sizeof(int)*2)+(sizeof(char)*2)+strlen(bestandsnaam)+10;
    char *buffer = malloc(size);
    bzero(buffer, size);
    strcpy(buffer, "");
    
    char *savedir = fixServerBestand(sockfd, bestandsnaam);
    
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
    if (file > -1) close(file);
    
    int ret = STUK;
    if (waitForOk(*sockfd) == MOOI){
        ret = switchResult(sockfd, buffer);
    } 
    
    if(buffer){
        buffer = NULL;
    }
    if(savedir){
        savedir = NULL;
    }
    return ret;
    
}

/**
 * Functie verstuurt de modify-date van een bestand naar de server en
 * krijgt terug welke de nieuwste is.
 * @param sockfd socket waarop gecontroleerd moet worden
 * @param bestandsnaam bestandsnaam van bestand dat gecontroleerd moet worden
 * @return 0 if succesvol. -1 if failed.
 */
int ModifyCheckFile(int* sockfd, char* bestandsnaam){
    char *path = malloc(strlen(bestandsnaam) + 1);
    struct stat bestandEigenschappen;
    bzero(path, strlen(bestandsnaam));
    char* buffer = malloc(BUFFERSIZE);
    char statusCode[4], seconden[40];
    int readCounter = 0;
    
    if (IS_CLIENT == STUK){
        int len = strlen(clients[*sockfd-4].username) + strlen("/userfolders/");
        path = bestandsnaam + len;
    } else {
        strcpy(path, bestandsnaam);
    }

    stat(bestandsnaam, &bestandEigenschappen);
    
    bzero(buffer, BUFFERSIZE);
    sprintf(seconden, "%i", (int) bestandEigenschappen.st_mtime);
    sprintf(statusCode, "%d", STATUS_MODCHK);
 
    sendPacket(*sockfd, STATUS_MODCHK, path, seconden, NULL);
    
    // Wacht op antwoord modifycheck van server
    if((readCounter = recv(*sockfd, buffer, BUFFERSIZE, 0)) < 0) {
        perror("Receive modififycheck result error");
        return STUK;
    }
    sendPacket(*sockfd, STATUS_OK, NULL);
    
    readCounter = switchResult(sockfd, buffer);
    
    free(buffer);      
    
    return MOOI;
}


int loopOverFiles(int* sockfd, char *path){
    char* savedir = fixServerBestand(sockfd, path);
    FTS *ftsp;
    FTSENT *p, *chp;
    
    char **dir = malloc(strlen(savedir));
    bzero(dir, strlen(savedir));
    dir[0] = savedir;
    
    int fts_options = FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR;
    
    if ((ftsp = fts_open((char * const *) dir, fts_options, 0)) == NULL) {
         perror("fts_open");
         return STUK;
    }
    
    /* Initialize ftsp with as many argv[] parts as possible. */
    chp = fts_children(ftsp, 0);
    if (chp == NULL) {
           return STUK;               /* no files to traverse */
    }

    while ((p = fts_read(ftsp)) != NULL) {
        if (p->fts_path[strlen(p->fts_path)-1] == '~'){ printf("Skipped %s because its a temp file!\n", p->fts_path); continue; }
                
        switch (p->fts_info) {
            case FTS_D:
                //directory
                printf("-- Searching in directory: %s\n", p->fts_path);
                //sendPacket(*sockfd, STATUS_MKDIR, p->fts_path, NULL);
                //waitForOk(*sockfd);
                break;
            case FTS_F:
                //file
                printf("-- Synchronising file: %s\n", p->fts_path);
                if(ModifyCheckFile(sockfd, p->fts_path) < 0){
                    printf("-- Synchronising of file: %s failed\n", p->fts_path);
                    continue;
                }
                break;
            default:
                break;
        }
    }
                
    if(dir){
        free(*dir);
        dir = NULL;
    }

    fts_close(ftsp);
    return MOOI;
}

int CreateFolder(int* sockfd, char* bestandsnaam){
    char* savedir = fixServerBestand(sockfd, bestandsnaam);
    
    char **path = malloc(strlen(savedir) + 100);
    bzero(path, strlen(savedir) + 100);
    int i, amount = transformWith(savedir, path, "/");
    if (amount > 0){
        char folderpath[BUFFERSIZE];
        bzero(folderpath, BUFFERSIZE);
            
        strcpy(folderpath, "");

        for(i=0; i<amount; i++){
            if (strcmp(path[i], "..") == 0) continue;
                
            strcat(folderpath, path[i]);
            strcat(folderpath, "/");
            mkdir(folderpath, S_IRWXU);
        }
    }
    
    if (path){
        free(*path);
        *path = NULL;
    }

    sendPacket(*sockfd, STATUS_OK, NULL);
    free(savedir);
    return MOOI;
}

/**
 * Checked of dit op de server uitgevoerd word, en stopt er de client folder voor.
 * @param sockfd
 * @param bestandsnaam
 * @return 
 */
char* fixServerBestand(int* sockfd, char* bestandsnaam){
    if (IS_CLIENT == MOOI) return bestandsnaam;
    //printf("/userfolders: %i, bestandsnaam: %i, username: %i", (int)strlen("/userfolders/ "), (int)strlen(bestandsnaam), (int)strlen(clients[*sockfd-4].username));
    char* savedir = malloc(strlen("/userfolders/ ") + strlen(bestandsnaam) + strlen(clients[*sockfd-4].username));
    bzero(savedir, (strlen("/userfolders/ ") + strlen(bestandsnaam) + strlen(clients[*sockfd-4].username)));
    strcpy(savedir, "");
    
    strcpy(savedir, "userfolders/");
    strcat(savedir, clients[*sockfd-4].username);
    strcat(savedir, "/");
    
    strcat(savedir, bestandsnaam);
    
    return savedir;
}

int sendPacket(int fd, int packet, ...){
    char *info = malloc(BUFFERSIZE);
    bzero(info, BUFFERSIZE);
    
    char *p = malloc(BUFFERSIZE);
    bzero(p, BUFFERSIZE);
    
    sprintf(p, "%i", packet);   
    
    va_list ap;
    char* i;
    
    va_start(ap, packet);
    
    for (i = p; i != NULL; i = va_arg(ap, char *)){
//        if(((unsigned char)i[strlen(i)-1]) == 9 || ((unsigned char)i[strlen(i)-1]) == 16){
//            i[strlen(i)-1] = '\0';
//        }    
//        if(((unsigned char)i[strlen(i)-2]) == 9 || ((unsigned char)i[strlen(i)-2]) == 16){
//            i[strlen(i)-2] = '\0';
//        }   
//        if(((unsigned char)i[strlen(i)-1]) == 9 || ((unsigned char)i[strlen(i)-1]) == 16){
//            i[strlen(i)-1] = '\0';
//        }
        sprintf(p, "%s", i);
        strcat(info, p);
        strcat(info, ":");
        bzero(p, BUFFERSIZE);

    }
    va_end(ap);
    info[strlen(info)-1] = '\0';
     
    int bytes;
    
    if((bytes=send(fd, info, strlen(info),0)) < 0){
        perror("send");
        return STUK;
    }
    printf("Send packet: %i, data: \"%s\"(bytes: %i)\n", packet, info, (int)strlen(info));
    free(info);
    free(p);
    
    return MOOI;
}

/**
 * Functie om nummer om te zetten naar string
 * @param number integer nummer
 * @return de meegegeven integer als string
 */
char *toString(int number){
    char *nr = malloc(20);
    bzero(nr, 10);
    
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
    bzero(pID, sizeof(int));
    
    sprintf(pID, "%i", STATUS_EOF);
    strcat(to, pID);
    strcat(to, ":EOF");
    
    free(pID);
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
    char *buffer = malloc(10);
    bzero(buffer, 10);
    if((recv(sockfd, buffer, 10, 0)) < 0) {
        perror("Receive OK error");
        free(buffer);
        return STUK;
    }
    
    if(switchResult(&sockfd, buffer) != STATUS_OK){
        free(buffer);
        printf("WaitForOK(): SwitchResult != STATUS_OK");
        return STUK;
    }
    
    free(buffer);
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
    bzero(temp, SHA256_DIGEST_LENGTH);
    
    // Initialiseer struct
    SHA256_Init(&ctx);
    
    printf("password: %s\n", password);
    // Plaats wachtwoord in struct 
    SHA256_Update(&ctx, password,  strlen(password));

    // Voeg salt en wachtwoord samen
    SHA256_Update(&ctx, salt, strlen(salt)); 
    
    // Genereer de hash
    SHA256_Final(temp, &ctx);
    
    // Maak van Hex een String
    convertHashToString(to, temp);
    free(temp);
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
    bzero(temp, max);
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
