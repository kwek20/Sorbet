/* 
 * File:   pfcclient.c
 * Author: Bartjan Zondag & Kevin Rosendaal
 *
 */

/*
 * Deze client kan een file uploaden naar de PFC server
 */

#include "pfcclient.h"

int sockfd, bestandfd;
struct sockaddr_in serv_addr;

int main(int argc, char** argv) {

    // Usage: pfcclient /tmp/test.txt 192.168.1.1
    argc = 3;
    argv[1] = "icon.xpm";
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
   FileNaarServer();
   
   return 0;
}

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

int ConnectNaarServer(){
    
    if((connect(sockfd,(struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0){
        perror("Connect error:");
        return -1;
    }

    printf("connectie succesvol\n");
    return 0;
}

/*
 * Functie geef aan dat je een bestand wilt uploaden
 */

int FileNaarServer(){
    
    char buffer[BUFFERGROOTE];
    int readCounter = 0;

    strcpy(buffer,"300:icon.xpm"); //test.txt wordt argv[1] zonder map structuur
    
    if((send(sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Send metadata error:");
        return -1;
    }
    
    if((recv(sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Receive metadata OK error:");
        return -1;
    }

    printf("%s\n", buffer);

    if(strcmp(buffer, "100") != 0){return -1;}
        
    printf("Meta data succesvol verstuurd en ok ontvangen\n");

    /*
     * Lees gegevens uit een bestand. Zet deze in de buffer. Stuur buffer naar server.
     * Herhaal tot bestand compleet ingelezen is.
     */
    while((readCounter = read(bestandfd, &buffer, BUFFERGROOTE)) > 0){
        if((send(sockfd, buffer, readCounter, 0)) < 0) {
            perror("Send file error:");
            return -1;
        }
        
        printf("pakket verstuurd! %i\n", readCounter);

        if((recv(sockfd, buffer, readCounter, 0)) < 0) {
            perror("Receive metadata OK error:");
            return -1;
        }
        
        if(strcmp(buffer, "100") != 0){return -1;}
    }

    if(readCounter < 0){
        return -1;
    }

    /*
     * Bestand klaar met versturen. Geef dit aan aan server doormiddel van 101:EOF
     */
    strcpy(buffer,"101:EOF");

    if((send(sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Send 101 error:");
        return -1;
    }

    if((recv(sockfd, buffer, strlen(buffer), 0)) < 0) {
        perror("Receive OK error:");
        return -1;
    }

    printf("EOF verstuurd en ok ontvangen. verbinding wordt verbroken\n");

    close(sockfd);
    close(bestandfd);
    
    return 0;
}