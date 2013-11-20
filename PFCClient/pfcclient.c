/* 
 * File:   pfcclient.c
 * Author: Bartjan Zondag & Kevin Rosendaal
 *
 * Created on November 19, 2013, 10:37 AM
 * Changed on November 19, 2013, 12:36 AM
 * Changed on November 19, 2013, 12:40 AM
 */

/*
 * Deze client kan een file uploaden naar de PFC server
 */

#include "pfcclient.h"

int main(int argc, char** argv) {

    //pfcclient /tmp/test.txt 192.168.1.1
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
   char buffer[BUFFERGROOTE];

   if((int ServerGegevens(argv[2], &serv_addr)) < 0){
       exit();
   }
   
}
/*
 * Functie serv_addr vullen  
 */

int ServerGegevens(char* ip, struct sockaddr_in *serv_addr){

    //memsets
   memset(&serv_addr, '\0', sizeof (serv_addr));


   /*
    * Vul juiste gegevens in voor server in struct serv_addr
    */
   serv_addr.sin_family = AF_INET;

   if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton error occured:");
        return(-1);
   }

   serv_addr.sin_port = 2200;

   return 0;
}

/*
 * Functie controleerd of bestand bestaat
 */

/*
 * Functie maak verbinding met de server
 *
 *
 */

/*
 * Functie geef aan dat je een bestand wilt uploaden
 */