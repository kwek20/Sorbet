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

   ConnectNaarServer(&sockfd);
   if(ModifyCheckClient(&sockfd, argv[1]) < 0){
       printf("error bij ModifyCheckClient\n");
       exit(EXIT_FAILURE);
   }
   
   return 0;
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

/* 
 * client verstuurd | 301:bestandnaam:datemodi
 * client ontvangd | 401 | ga naar FileTransferSend()
 * client ontvangd | 402 | ga naar FileTransferRecieve()
 * client ontvangd | 2XX | Error
 */