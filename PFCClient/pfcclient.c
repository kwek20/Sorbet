/* 
 * File:   pfcclient.c
 * Author: Bartjan Zondag & Kevin Rosendaal
 *
 */

/*
 * Deze client kan een file uploaden naar de PFC server
 */

#include "pfc.h"

int sockfd, bestandfd;
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

int pfcClient(char** argv){

   //Check if file exists
   if((BestaatDeFile(argv[1])) < 0){
       printf("Geef een geldige file op\n");
       exit(1);
   }
    
   //open file
   if((OpenBestand(argv[1])) < 0){
       exit(1);
   }

   if((ServerGegevens(argv[2])) < 0){
       exit(1);
   }
   
   // Create socket
   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
       perror("Create socket error:");
       exit(1);
   }

   ConnectNaarServer();
   //ModifyCheckClient(argv[1]);
   FileTransferSend(argv[1]);
   
   return 0;
}


/*
 * Functie serv_addr vullen
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

/*
 * Functie maak verbinding met de server
 */
int ConnectNaarServer(){
    
    if((connect(sockfd,(struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0){
        perror("Connect error:");
        return -1;
    }

    printf("connectie succesvol\n");
    return 0;
}

/* 
 * client verstuurd | 301:bestandnaam:datemodi
 * client ontvangd | 401 | ga naar FileTransferSend()
 * client ontvangd | 402 | ga naar FileTransferRecieve()
 * client ontvangd | 2XX | Error
 */