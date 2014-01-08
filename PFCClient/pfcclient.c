/* 
 * File:   pfcclient.c
 * Author: Bartjan Zondag & Kevin Rosendaal & Brord van Wierst
 * 
 */

/*
 * Deze client kan een file uploaden/downloaden naar/van de PFC server.
 * Dit gebeurd via een synchronisatie protocol. 
 */

#include "pfc.h"
#include <fts.h>

struct sockaddr_in serv_addr;

int pfcClient(char** argv);
int ServerGegevens(char* ip);
int ConnectNaarServer(int* sockfd);
int SendCredentials(int* sockfd);
int ModifyCheckClient(int* sockfd, char* bestandsnaam);
int loopOverFilesS(char **path, int* sockfd);


struct timeval globalTv;
time_t globalStarttime, globalEndtime, globalTime;

int main(int argc, char** argv) {
    
    // Usage: pfcclient /tmp/test.txt 192.168.1.1
    //argc = 3;
    //argv[1] = "test.txt";
    argv[2] = "127.0.0.1"; //moet nog veranderd worden
    
    IS_CLIENT = MOOI;
    pfcClient(argv);
    
    return (EXIT_SUCCESS);
}

/**
 * Hoofdprogramma voor de pfcClient
 * @param argv argumenten van commandline
 * @return 0 if succesvol. -1 if failed.
 */
int pfcClient(char** argv){
   int sockfd;

   if((ServerGegevens(argv[2])) < 0){
       exit(EXIT_FAILURE);
   }
   
   
   if (argv[1] == NULL){
       char cwd[1024];
       getcwd(cwd, sizeof(cwd));
       puts(cwd);
       argv[1] = cwd;
   }
   
   // Create socket
   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
       perror("Create socket error:");
       exit(EXIT_FAILURE);
   }
   
   printStart();
   if(ConnectNaarServer(&sockfd) != MOOI){exit(EXIT_FAILURE);}
   if(SendCredentials(&sockfd) != MOOI){exit(EXIT_FAILURE);}
   
   exit(loopOverFilesS(argv + 1, &sockfd));
}

int loopOverFilesS(char **path, int* sockfd){
    FTS *ftsp;
    FTSENT *p, *chp;
    int fts_options = FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR;
    int totalSize = 0;
    
    struct stat st;
    
    if (DEBUG >= 1){
        gettimeofday(&globalTv,NULL);
        globalStarttime = globalTv.tv_sec;
    }
    
    if ((ftsp = fts_open((char * const *) path, fts_options, 0)) == NULL) {
         perror("fts_open");
         return STUK;
    }
    
    /* Initialize ftsp with as many argv[] parts as possible. */
    chp = fts_children(ftsp, 0);
    if (chp == NULL) {
           return STUK;               /* no files to traverse */
    }

    while ((p = fts_read(ftsp))) {
        
        if (p->fts_path[strlen(p->fts_path)-1] == '~'){ printf("Skipped %s because its a temp file!\n", p->fts_path); continue; }
                
        switch (p->fts_info) {
            case FTS_D:
                //directory
                printf("-- Searching in directory: %s\n", p->fts_path);
                break;
            case FTS_F:
                //file
                stat(p->fts_path, &st);
                int size = st.st_size;
                totalSize += size;
                printf("-- Synchronising file: [%s] | Size in Bytes: [%i]\n", p->fts_path, size);
                if(ModifyCheckFile(sockfd, p->fts_path) < 0){
                    printf("-- Synchronising of file: %s failed ;-(\n", p->fts_path);
                }
                break;
            default:
                break;
        }
    }
    
    fts_close(ftsp);
    sendPacket(*sockfd, STATUS_SYNC, path[0], NULL);
    char* buffer = malloc(BUFFERSIZE);
    int ret = MOOI, readCounter = 0;
    bzero(buffer, BUFFERSIZE);
    
    for ( ;; ){
        if((readCounter = recv(*sockfd, buffer, BUFFERSIZE, 0)) < 0) {
            //printf("%s(%i, %i)\n", buffer, readCounter, *sockfd);
            perror("Receive modififycheck result error");
            ret = STUK;
            break;
        } else if (readCounter == 0){
            break;
        }
        
        readCounter = switchResult(sockfd, buffer);
        bzero(buffer, BUFFERSIZE);
        
        if (readCounter == STUK) break;
    }
    
    puts("\n-- Done --\n");
    
    if (DEBUG >= 1){
    gettimeofday(&globalTv,NULL);
    globalEndtime = globalTv.tv_sec;
    globalTime = globalEndtime - globalStarttime;
    
    printf("Summary:\n");
    printf("Total running time: %u second(s)\n", (unsigned int) globalTime);
    printf("Total size of directory including subdirectories: %i MB\n\n", totalSize/1000000);
    }
    
    if(ret == STUK) puts("There was an error upon exit");
    
    return ret;
}

/**
 * Functie serv_addr vullen
 * @param ip Ascii ip van server 
 * @return 0 if succesvol. -1 if failed.
 */
int ServerGegevens(char* ip){

    //memsets
   memset(&serv_addr, '\0', sizeof (serv_addr));
   /*
    * Vul juiste gegevens in voor server in struct serv_addr
    */
   serv_addr.sin_family = AF_INET;

   /*
    * This function converts the character string ip into a network address using the structure of AF_INET
    * and copies it to serv_addr.sin_addr
    */
   if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton error occured:");
        return(-1);
   }
   
   serv_addr.sin_port = htons(NETWERKPOORT);

   return 0;
}

/**
 * Functie maak verbinding met de server
 * @param sockfd file descriptor voor de server
 * @return 0 if succesvol. -1 if failed.
 */
int ConnectNaarServer(int* sockfd){
    
    if((connect(*sockfd,(struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0){
        perror("Connect error:");
        return -1;
    }
    
    printf("Connectie succesvol\n");
    return 0;
}

/**
 * De credentials 
 * @param sockfd
 * @return 
 */
int SendCredentials(int* sockfd){
    
     /*
     * client verstuurd verzoek om in te loggen (302:username:password)
     * server verstuurd 404 als dit mag of 203 als dit niet mag
     */
    char *username, *password, *buffer;
    char hex[SHA256_DIGEST_LENGTH*2+1];
    int sR = 0; //switchResult

    
    for(;;){
        
        buffer = invoerCommands("Username: ", 50);
        username = malloc(100);
        strcpy(username,buffer);
        
        memset(buffer, 0 , strlen(buffer));
        
        buffer = invoerCommands("Password: ", 50);
        password = malloc(100);
        strcpy(password,buffer);
        hashPassword(password, FIXEDSALT, hex);
        
        memset(buffer, 0 , strlen(buffer));
        
        printf("ready to send\n");
        sendPacket(*sockfd, STATUS_AUTH, username, hex, NULL);
        if((recv(*sockfd, buffer, BUFFERSIZE, 0)) < 0) {
            perror("Receive metadata OK error");
            return STUK;
        }

        sR = switchResult(sockfd, buffer);
        
        free(username); free(password), free(buffer);
        switch(sR){
            case STUK: return STUK;
            case STATUS_AUTHFAIL: printf("Username or Password incorrect\n"); continue;
            case STATUS_AUTHOK: printf("Succesvol ingelogd\n"); return MOOI;
        }
    }
    
    return STUK;
}

/**
 * Deze functie vraagt input van de gebruiker. Er wordt tekst geprint voor de invoer. Bij een enter zal deze functie opnieuw om invoer vragen.
 * @param tekstVoor de tekst die geprint wordt voor het commando
 * @param aantalChars aantal characters dat de invoer maximaal mag hebben
 * @return returns buffer pointer
 */
char* invoerCommands(char* tekstVoor, int aantalChars){
    char* buffer;
    for(;;){
        printf("%s",tekstVoor);
        buffer = getInput(aantalChars);
        if(!strcmp("",buffer)){
            continue;
        }
        return buffer;
    }
}