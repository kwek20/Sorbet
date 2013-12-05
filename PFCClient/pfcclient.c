/* 
 * File:   pfcclient.c
 * Author: Bartjan Zondag & Kevin Rosendaal
 * 
 */

/*
 * Deze client kan een file uploaden/downloaden naar/van de PFC server.
 * Dit gebeurd via een synchronisatie protocol. 
 */

#include "pfc.h"

struct sockaddr_in serv_addr;

int SendCredentials(int* sockfd);

int main(int argc, char** argv) {

    // Usage: pfcclient /tmp/test.txt 192.168.1.1
    argc = 3;
    argv[1] = "test.txt";
    argv[2] = "127.0.0.1"; //moet nog veranderd worden
    
    pfcClient(argv);
    
    /*
     * invoer bestand
     * invoer ip server
     */
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
   
   // Create socket
   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
       perror("Create socket error:");
       exit(EXIT_FAILURE);
   }
   
   printStart();

   if(ConnectNaarServer(&sockfd) != MOOI){exit(EXIT_FAILURE);}
   if(SendCredentials(&sockfd) != MOOI){exit(EXIT_FAILURE);}
   if(ModifyCheckClient(&sockfd, argv[1]) < 0){
       printf("error bij ModifyCheckClient\n");
       exit(EXIT_FAILURE);
   }
   exit(EXIT_FAILURE);
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
 * Functie verstuurt de modify-date van een bestand naar de server en
 * krijgt terug welke de nieuwste is.
 * @param sockfd socket waarop gecontroleerd moet worden
 * @param bestandsnaam bestandsnaam van bestand dat gecontroleerd moet worden
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
        //printf("%s(%i)\n", buffer, readCounter);
        perror("Receive modififycheck result error");
        return STUK;
    }
    sendPacket(*sockfd, STATUS_OK, NULL);
    switchResult(sockfd, buffer);
    
    return MOOI;
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
    int sR = 0; //switchResult

    
    for(;;){
        
        printf("Username: ");
        buffer = getInput(50);
        username = malloc(strlen(buffer));
        strcpy(username,buffer);
        
        memset(buffer, 0 , strlen(buffer));
        
        printf("Password: ");
        buffer = getInput(50);
        password = malloc(strlen(buffer));
        strcpy(password,buffer);

        memset(buffer, 0 , strlen(buffer));
        
        printf("ready to send\n"); //BRORD !!!! WERKT NIET
        sendPacket(*sockfd, STATUS_AUTH, username, password, NULL);
        if((recv(*sockfd, buffer, BUFFERSIZE, 0)) < 0) {
            perror("Receive metadata OK error");
            return STUK;
        }

        sR = switchResult(sockfd, buffer);
        switch(sR){
            case STUK: return STUK;
            case STATUS_AUTHFAIL: printf("Username or Password incorrect\n"); continue;
            case STATUS_AUTHOK: printf("Succesvol ingelogd\n"); return MOOI;
        }
    }
    
    return STUK;
}