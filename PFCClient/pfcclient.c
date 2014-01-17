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
#include <netdb.h>
#include <sys/inotify.h>

#define HOSTADDR "kevin.rosendaal.net"
#define MAXWATCHDIR 20 //Hoeveelheid directories die maximaal gemonitord kunnen worden
#define EVENT_SIZE (sizeof (struct inotify_event))
#define EVENT_BUF_LEN (1024 * ( EVENT_SIZE + 16))

//Struct om watch directory naam en id bij te houden
typedef struct watchDir{
    int wd;
    char* directoryname;
} watchDir;

struct sockaddr_in serv_addr;
watchDir wd[MAXWATCHDIR]; //Array van watch descriptors voor folder/file monitoring

int pfcClient(char** argv);
int ServerGegevens(char* ip);
int ConnectNaarServer(int* sockfd);
int SendCredentials(SSL* ssl);
int ModifyCheckClient(SSL* ssl, char* bestandsnaam);
int loopOverFilesS(char **path, SSL* ssl);
int monitorD(char** argv);

int recursiveFolderCheck(int *fd, int *lastDir, char* folder);
int controleFolder(int *fd, int *lastDir, char *foldername);
int getWD(int wdesc, int lastDir, char childFolder[]);

// SSL Prototypes
SSL_CTX* initCTX();
SSL* initSSL(int sockfd);

struct timeval globalTv;
time_t globalStarttime, globalEndtime, globalTime;

int main(int argc, char** argv) {
    struct hostent *hostname;
    struct in_addr **ipadres;
    char* ip;

    hostname = gethostbyname(HOSTADDR);
    ipadres = (struct in_addr **) hostname->h_addr_list;
    ip = inet_ntoa(*ipadres[0]);

    argv[2] = ip;

    monitorD(argv);
    //pfcClient(argv);
    
    return (EXIT_SUCCESS);
}

int controleFolder(int *fd, int *lastDir, char *foldername){
    if(*lastDir < MAXWATCHDIR){
        if(strcmp(&foldername[strlen(foldername)-1],"/") == 0){
            wd[*lastDir].directoryname = malloc(strlen(foldername+1));
            strcpy(wd[*lastDir].directoryname,foldername);
        }else{
            wd[*lastDir].directoryname = malloc(strlen(foldername)+2);
            strcpy(wd[*lastDir].directoryname,foldername);
            strcat(wd[*lastDir].directoryname,"/");
        }

        if((wd[*lastDir].wd = inotify_add_watch(*fd, wd[*lastDir].directoryname, IN_ALL_EVENTS)) < 0){
            perror("controleFolder: wd[lastDir] went wrong");
            return STUK;
        }
        
        if(DEBUG >= 1)printf("wd[%i] Directoryname: %s\n", *lastDir, wd[*lastDir].directoryname);
        *lastDir = *lastDir + 1;
        if(DEBUG >= 1)printf("lastDir %i\n",*lastDir);
        
    }
    return MOOI;
}

int recursiveFolderCheck(int *fd, int *lastDir, char* folder){
    
    FTS *ftsp;
    FTSENT *p, *chp;
    
    char **dir = malloc(strlen(folder)+1);
    bzero(dir, strlen(folder)+1);
    dir[0] = folder;
    
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
        if(p->fts_info == FTS_D && strcmp(p->fts_name,"") != 0){
//            printf("fts_path: %s\n",p->fts_path);
            wd[*lastDir].directoryname = malloc(strlen(p->fts_path)+2);
            strcpy(wd[*lastDir].directoryname,p->fts_path);
            strcat(wd[*lastDir].directoryname,"/");
            
            if((wd[*lastDir].wd = inotify_add_watch(*fd, wd[*lastDir].directoryname, IN_ALL_EVENTS)) < 0){
                perror("controleFolder: wd[lastDir] went wrong");
                return STUK;
            }
            *lastDir = *lastDir + 1;
        }
        
    }
//    
//    if(folder){
//        free(folder);
//        folder = NULL;
//    }

    fts_close(ftsp);
    
    return MOOI;
}

/**
 * Get WDesc of child folder based on pathname
 * @param wdesc descripter of parent folder
 * @param lastDir lastDir defined in array
 * @param childFolder folder name that is inside the parent
 * @return wdesc of child in array. or stuk if broken.
 */
int getWD(int wdesc, int lastDir, char childFolder[]){
    
    int i = 0;
    char *pathName;
    
    pathName = malloc(strlen(wd[wdesc].directoryname) + strlen(childFolder) + 2);
    strcpy(pathName, wd[wdesc].directoryname);
    strcat(pathName,childFolder);
    strcat(pathName,"/");
    if(DEBUG >= 1)printf("pathName: %s\n",pathName);
    for(i = 0;i < lastDir; i++){
        if(strcmp(wd[i].directoryname, pathName) == 0){
            free(pathName);
            return i;
        }
    }
    
    free(pathName);
    return STUK;
}

/**
     * Monitord de opgegeven directory op wijzigingen zoals deleted/modify/created
     * @param argv argumenten van commandline
     * @return 0 if succesvol. -1 if failed.
     */
int monitorD(char** argv){
        
    int *fd = malloc(sizeof(int)); // FD van de watched directories
    int length, i = 0, *lastDir = malloc(sizeof(int)), tempCheck = 0, tempWDesc = 0;
    char buffer[EVENT_BUF_LEN], tempFolder[1024];
    uint32_t moveCookie;
    
    bzero(lastDir,sizeof(int));
    bzero(fd,sizeof(int));
    
    if((*fd = inotify_init()) < 0){
        perror( "inotify_init" );
        return STUK;
    }
    
    if((tempCheck = controleFolder(fd, lastDir, argv[1])) < 0){
        if(DEBUG >= 1)printf("Folder Check went wrong in %s | lastDir %i\n", wd[*lastDir].directoryname, *lastDir);
        tempCheck = 0;
    }else{
        tempCheck = 0;
    }
    
    if(DEBUG >= 1)printf("wd[%i].directoryname: %s\n",*lastDir, wd[*lastDir-1].directoryname);
    if((tempCheck = recursiveFolderCheck(fd, lastDir, wd[*lastDir-1].directoryname)) < 0){
        tempCheck = 0;
    }else{
        tempCheck = 0;
    }
    
    if(DEBUG >= 1){
        for(i = 0; i < *lastDir; i++){
            printf("wd[%i].directoryname: %s\n",i,wd[i].directoryname);
        }
    }
    for(;;){
        i = 0;
        bzero(buffer,EVENT_BUF_LEN);
        
        if((length = read(*fd, buffer, EVENT_BUF_LEN)) < 0){
            perror("read watchfd went wrong");
            return STUK;
        }
        
        
        while(i<length){
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            if (event->len && strcmp(strtok(event->name,"-"),".goutputstream") != 0 && strcmp(&event->name[strlen(event->name)-1],"~") != 0){
                if(event->mask & IN_CREATE){
                    if(event->mask & IN_ISDIR){
                        if(DEBUG >= 1)printf("New directory %s%s created.\n", wd[(event->wd)-1].directoryname, event->name);
                        
                        strcpy(tempFolder, wd[(event->wd)-1].directoryname);
                        strcat(tempFolder, event->name);
                        controleFolder(fd, lastDir, tempFolder);
                        if(DEBUG >= 1)printf("folder %s created | lastDir %i\n", wd[*lastDir-1].directoryname, *lastDir);
                    }else{
                        if(DEBUG >= 1)printf("New file %s%s created.\n", wd[(event->wd)-1].directoryname, event->name);
                    }                    
                }
                if(event->mask & IN_DELETE){
                    if(event->mask & IN_ISDIR){
                        if(DEBUG >= 1)printf("directory %s%s deleted.\n", wd[(event->wd)-1].directoryname, event->name);
                    }else{
                        if(DEBUG >= 1)printf("file %s%s deleted.\n", wd[(event->wd)-1].directoryname, event->name);
                    }  
                }
                if(event->mask & IN_MODIFY){
                    if(event->mask & IN_ISDIR){
                        if(DEBUG >= 1)printf("directory %s modified.\n", event->name);
                    }else{
                        if(DEBUG >= 1)printf("file %s modified.\n", event->name);
                    }                    
                }
                if(event->mask & IN_MOVED_FROM){
                    if(event->mask & IN_ISDIR){
                        if(DEBUG >= 1)printf("directory %s moved old name.\n", event->name);
                        if((tempWDesc = getWD((event->wd-1), *lastDir, event->name)) < 0){
                                printf("getWD went wrong tempWDesc %i\n",tempWDesc);
                        }
                    }else{
                        if(DEBUG >= 1)printf("file %s moved old name.\n", event->name);
                    }
                    moveCookie = event->cookie;                    
                }
                if(event->mask & IN_MOVE_SELF){
                    if(event->mask & IN_ISDIR){
                        if(DEBUG >= 1)printf("directory %s MOVE SELF.\n", event->name);
                    }else{
                        if(DEBUG >= 1)printf("file %s MOVE SELF.\n", event->name);    
                    }                    
                }
                if(event->mask & IN_MOVED_TO && moveCookie == event->cookie){
                    if(event->mask & IN_ISDIR){
                        if(DEBUG >= 1)printf("directory %s Renamed.\n", event->name);
                        
                        //verwijderen
                        inotify_rm_watch(*fd, tempWDesc);
                        strcpy(tempFolder, wd[(event->wd)-1].directoryname);
                        strcat(tempFolder, event->name);
                        controleFolder(fd, &tempWDesc, tempFolder);
                        if(DEBUG >= 1)printf("directory %s Renamed path & wd %i wdtemp %i\n",wd[tempWDesc-1].directoryname,wd[tempWDesc-1].wd, tempWDesc);
                    }else{
                        if(DEBUG >= 1)printf("file %s Renamed.\n", event->name);
                    }
                    bzero(&moveCookie, sizeof(uint32_t));
                }else if(event->mask & IN_MOVED_TO){
                    if(event->mask & IN_ISDIR){
                        if(DEBUG >= 1)printf("directory %s Moved into folder.\n", event->name);
                        strcpy(tempFolder, wd[(event->wd)-1].directoryname);
                        strcat(tempFolder, event->name);
                        controleFolder(fd, lastDir, tempFolder);
                    }else{
                        if(DEBUG >= 1)printf("file %s Moved into folder.\n", event->name);
                    }
                }
            }
            i += EVENT_SIZE + event->len;
        }
    }
    
    /*
     * //Maak een verbinding met de server
     * geef alle folders op voor inotify via add watch
     * 
     * loop
     * zijn er nieuwe folders bijgekomen? zo ja sync deze en voeg ze toe aan de watch list
     * monitor de fd die hieruit komt en roep pfcClient aan met de file/folder die veranderd is.
     * ga naar loop
     */
    
    
    return MOOI;
}


/**
 * Hoofdprogramma voor de pfcClient
 * @param argv argumenten van commandline
 * @return 0 if succesvol. -1 if failed.
 */
int pfcClient(char** argv){
   int sockfd;
   SSL* ssl;

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
   ssl = initSSL(sockfd);
   if(SendCredentials(ssl) != MOOI){exit(EXIT_FAILURE);}
   
   exit(loopOverFilesS(argv + 1, ssl));
   
   SSL_shutdown(ssl);
   SSL_free(ssl);
   close(sockfd);
}

SSL* initSSL(int sockfd)
{
    SSL_CTX* CTX;
    SSL* ssl;
    
    SSL_library_init();
    
    CTX = initCTX();
    ssl = SSL_new(CTX);
    SSL_set_fd(ssl, sockfd);
    
    if (SSL_connect(ssl) == STUK)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
  
    return ssl;
}

SSL_CTX* initCTX()
{
    const SSL_METHOD *method;
    SSL_CTX* CTX;
    
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLSv1_client_method();  /* Create new client-method instance */
    CTX = SSL_CTX_new(method);   /* Create new context */
    if ( CTX == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    
    return CTX;
}

int loopOverFilesS(char **path, SSL* ssl){
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
                if(ModifyCheckFile(ssl, p->fts_path) < 0){
                    printf("-- Synchronising of file: %s failed ;-(\n", p->fts_path);
                }
                break;
            default:
                break;
        }
    }
    
    fts_close(ftsp);
    sendPacket(ssl, STATUS_SYNC, path[0], NULL);
    char* buffer = malloc(BUFFERSIZE);
    int ret = MOOI, readCounter = 0;
    bzero(buffer, BUFFERSIZE);
    
    for ( ;; ){
        if((readCounter = SSL_read(ssl, buffer, BUFFERSIZE)) < 0) {
            //printf("%s(%i, %i)\n", buffer, readCounter, *sockfd);
            perror("Receive modififycheck result error");
            ret = STUK;
            break;
        } else if (readCounter == 0){
            break;
        }
        
        readCounter = switchResult(ssl, buffer);
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
int SendCredentials(SSL* ssl){
    
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
        sendPacket(ssl, STATUS_AUTH, username, hex, NULL);
        if((SSL_read(ssl, buffer, BUFFERSIZE)) < 0) {
            perror("Receive metadata OK error");
            return STUK;
        }

        sR = switchResult(ssl, buffer);
        
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
