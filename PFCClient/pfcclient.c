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

int initEncrypt();
char* getRandom(int size);
char* newPwd(int pwdSize);

int main(int argc, char** argv) {
    
    // Usage: pfcclient /tmp/test.txt 192.168.1.1
    //argc = 3;
    //argv[1] = "test.txt";
    argv[2] = "127.0.0.1"; //moet nog veranderd worden
    
    IS_CLIENT = MOOI;
    
//    initEncrypt();
//    
//    char* from = malloc(100);
//    strcpy(from, "hallo dit is een test");
//    
//    char* to = malloc(BUFFERSIZE);
//    
//    int bytes = aes_encrypt(from, to);
//    
//    printf("Original: %s\n", from);
//    printf("New: %s, length: %i\n", to, bytes);
    
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

   if(initEncrypt() != MOOI){
       puts("Could not load private encryption key");
       exit(EXIT_FAILURE);
   }

   if((ServerGegevens(argv[2])) < 0){
       puts("Could not load server details");
       exit(EXIT_FAILURE);
   }
   
   
   if (argv[1] == NULL){
       char cwd[1024];
       getcwd(cwd, sizeof(cwd));
       argv[1] = cwd;
   }
   
   // Create socket
   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
       perror("Create socket error");
       exit(EXIT_FAILURE);
   }
   
   printStart();
   if(ConnectNaarServer(&sockfd) != MOOI){exit(EXIT_FAILURE);}
   if(SendCredentials(&sockfd) != MOOI){exit(EXIT_FAILURE);}
   
   exit(loopOverFilesS(argv + 1, &sockfd));
}

int initEncrypt(){
    int readSize=0, keyfd=0, pwdSize = 8;
    char *buffer = malloc(pwdSize+1);
    //char *salt = "SoRbEt";
    char *pwd = malloc(pwdSize+1);
    strcpy(pwd, "");
    if((keyfd = open("secret.key",O_RDWR)) == STUK){
        pwd = newPwd(pwdSize);
    } else {
        readSize = read(keyfd, buffer, pwdSize);
        if (readSize == 8){
            strcat(pwd, buffer);
        } else {
            pwd = newPwd(pwdSize);
        }
        close(keyfd);
    }              
    unsigned int pwd_len = strlen(pwd);
    if(aes_init((unsigned char*)pwd,pwd_len,(unsigned char*) FIXEDSALT)){                /* Generating Key and IV and initializing the EVP struct */
        perror("\n Error, Cant initialize key and IV");
        return STUK;
    }
    free(pwd);
    free(buffer);
    return MOOI;
}

char* getRandom(int size){
    char *pwd = malloc(size+1);
    strcpy(pwd, "");
    
    char *buff = malloc(size+1);
    int currentSize = 0, randomfd, readSize;
    
    while (currentSize < size){
        if((randomfd = open("/dev/urandom", O_RDONLY)) == -1){
            perror("\n Error,Opening /dev/random::");
            return pwd;
        } else {
            if((readSize = read(randomfd,buff,size)) == -1) {
                perror("\n Error,reading from /dev/random::");
                return pwd;
            }
            currentSize += readSize;
            strcat(pwd, buff);
            close(randomfd);
        }
    }
    return pwd;    
}

char* newPwd(int pwdSize){
    int keyfd;
    char* pwd = malloc(pwdSize+1);
    
    if((keyfd = open("secret.key",O_RDWR|O_CREAT, 0777)) == STUK){
        perror("\n Could not create secret.key");
        return "";
    } else {
        pwd = getRandom(pwdSize);
        keyfd = write(keyfd, pwd, pwdSize);
        close(keyfd);
    }
    return pwd;
}

int loopOverFilesS(char **path, int* sockfd){
    FTS *ftsp;
    FTSENT *p, *chp;
    int fts_options = FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR;
    
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
                printf("-- Synchronising file: [%s]\n", p->fts_path);
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
    
    puts("-- Done --");
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
        perror("Connect error");
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
        
        username = malloc(strlen(buffer)+1);
        bzero(username, strlen(buffer)+1);
        strcpy(username,buffer);
        
        bzero(buffer, strlen(buffer));
        
        buffer = invoerCommands("Password: ", 50);
        
        password = malloc(strlen(buffer)+1);
        bzero(password, strlen(buffer)+1);
        strcpy(password,buffer);
        
        hashPassword(password, FIXEDSALT, hex);
        
        free(buffer);
        buffer = malloc(BUFFERSIZE);
        
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
    char* buffer = malloc(aantalChars);
    for(;;){
        printf("%s",tekstVoor);
        buffer = getInput(aantalChars);
        if(!strcmp("",buffer)){
            continue;
        }
        return buffer;
    }
}
