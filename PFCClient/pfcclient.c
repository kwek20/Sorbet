/* 
 * File:   pfcclient.c
 * Author: Bartjan Zondag & Kevin Rosendaal
 *
 */

/*
 * Deze client kan een file uploaden naar de PFC server
 */

#include "pfcclient.h"

int sockfd;

int main(int argc, char** argv) {

    // Usage: pfcclient /tmp/test.txt 192.168.1.1
    argc = 3;
    argv[1] = "/tmp/test.txt";
    argv[2] = "192.168.1.1"; //moet nog veranderd worden
    
    pfcClient(argv);
    
    /*
     * invoer bestand
     * invoer ip server
     */
    return (EXIT_SUCCESS);
}

int pfcClient(char** argv){

   struct sockaddr_in serv_addr;

   if((ServerGegevens(argv[2], serv_addr)) < 0){
       exit(1);
   }
   
   // Create socket
   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
       perror("Create socket error:");
       return -1;
   }
   
   return 0;
}

/*
 * Functie serv_addr vullen
 */

int ServerGegevens(char* ip, struct sockaddr_in serv_addr){

    //memsets
   memset(&serv_addr, '\0', sizeof (serv_addr));


   /*
    * Vul juiste gegevens in voor server in struct serv_addr
    */
   serv_addr.sin_family = AF_INET;

   // Wat doet dit?
   if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton error occured:");
        return(-1);
   }
   
   serv_addr.sin_port = NETWERKPOORT;

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
 * Functie maak verbinding met de server
 */

int ConnectNaarServer(struct sockaddr_in *serv_addr){
    
    if(connect(sockfd,(struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        perror("Connect error:");
        return -1;
    }

    return 0;
}

/*
 * Functie geef aan dat je een bestand wilt uploaden
 */

int FileNaarServer(){
    
    char buffer[BUFFERGROOTE];
    
    buffer = "300:derp.txt";
    
    if((send(sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Send metadata error:");
        return -1;
    }
    
    if((recv(sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Receive metadata OK error:");
        return -1;
    }
    
    if(!strcmp(buffer, "100")){
        
        buffer = "Dit moet een bestand voorstellen";
        
        if((send(sockfd, buffer, strlen(buffer), 0)) < 0) {
            perror("Send file error:");
            return -1;
        }
        
        buffer = "101";
    
        if((send(sockfd, buffer, strlen(buffer), 0)) < 0) {
            perror("Send 101 error:");
            return -1;
        }
        
        if((recv(sockfd, buffer, strlen(buffer), 0)) < 0) {
            perror("Receive OK error:");
            return -1;
        }
        
        close(sockfd);
    }
}