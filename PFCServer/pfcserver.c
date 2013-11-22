/* 
 * File:   pfcserver.c
 * Author: FPC project group
 *
 * Created on November 19, 2013, 10:37 AM
 */

#include "pfcserver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <memory.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>


#define PORT 2200
#define MAX_CLI 10

void SIGexit(int sig);
void setupSIG();
void create(int *sock);
int transform(char *text, char** to);
int sendPacket(int sock, int packet, ...);

int cur_cli = 0;
int sock;

/*
 * 
 */
int main(int argc, char** argv) {
    int poort = PORT;
    if (argc > 1 && argv[1] != NULL){
        poort = atoi(argv[1]);
    }

    setupSIG();

    struct sockaddr_in server_addr;

    //clear data
    memset(&server_addr, '0' , sizeof(server_addr));

    //set family to ipv4, poort naar gegeven poort, en luister naar elk addres
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(poort);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket");
        return -1;
    }

    //maak bind via socket
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0){
        perror("bind");
        return -1;
    }

    //listen maken
    if (listen(sock, MAX_CLI) < 0){
        perror("listen");
        return -1;
    }
    
    printf("Private file cloud server ready!\nWere listening for clients on port \"%i\".\n", PORT);
    for ( ;; ){
        if(cur_cli < MAX_CLI){
            cur_cli++;
            pthread_t client;
            pthread_create(&client, NULL, (void*)create, &sock);
            pthread_detach(client);
        }
    }

    return (EXIT_SUCCESS);
}

void create(int *sock){
    int modus = 0, fd, rec;
    struct sockaddr_in client_addr;

    char buffer[BUFSIZ];
    socklen_t size = sizeof(client_addr);
    if ((fd = accept(*sock, (struct sockaddr *)&client_addr, &size)) < 0){
         perror("accept");
    }

    char *ip;
    ip = inet_ntoa(client_addr.sin_addr);
    int poort = htons(client_addr.sin_port);

    printf("-------------\nConnection accepted with client: %i\n", fd);
    printf("Ip nummer: %s\n", ip);
    printf("Port: %i\n", poort);
    printf("Ready to receive!\n");

    char **array = malloc(1);
    char *filename = malloc(1);
    int file = -1;

    for ( ;; ){
        if ((rec = recv(fd, buffer, sizeof(buffer),0)) < 0){
            perror("recv");
            printf("Client error! Stopping... \n");
        } else if (rec == 0){
            //connection closed
        } else {
            //good
            int values = 0;

            if (modus == 0){
                values = transform(buffer, array);

                if (values < 1){
                    //?? empty, shouldnt have happened
                } else {
                    modus = atoi(array[0]);
                    printf("Selected modus: %i\n", modus);
                    strcpy(filename, array[1]);

                    sendPacket(fd, 100);
                    if (modus == 300){
                        
                    }
                }
            } else {
                //hes in a modus? whatchu got for me
                if (rec < 50){
                    //want to stop?
                    char *eind = malloc(20);
                    strcpy(eind, "");

                    char *pID = malloc(20);
                    sprintf(pID, "%d", 101);

                    strcat(eind, pID);
                    strcat(eind, ":EOF");

                    if (strcmp(buffer, eind) == 0){
                        //stop data
                        puts("Stopping file transfer");
                        modus = 0;
                        close(file);
                        sendPacket(fd, 100);
                        //continue;
                        
                        //values = 0;
                        break;
                    }
                }

                if (file < 0){
                    file = open(filename, O_WRONLY | O_CREAT, 0666);
                }
                
                if (file != 0){
                    sendPacket(fd, 100);
                    //MOAR data
                    //save it all

                    printf("received data: %i\n", rec);
                    write(file, buffer, rec);
                } else {
                    perror("open");
                }
                memset(buffer, 0, sizeof(buffer));
            }
        }
    }

    printf("Client stopped\n");
    close(fd);
    cur_cli--;
}

int transform(char *text, char** to){
    char *temp;

    temp = strtok(text, ":");
    int colon = 0;

    while (temp != NULL || temp != '\0'){
        to[colon] = temp;
        temp = strtok(NULL, ":");
        colon++;
    }

    return colon;
}

int sendPacket(int fd, int packet, ...){
/*
    puts("test4");
    char *arg;

    char *info = malloc(20);
    info = "";

    //     strcat(info, (char *) packet);

    printf("[%s]", info);
    puts("bleh");
    
    strcat(info, ":");

    puts("test5");
    printf("%s\n", info);

    va_list ap;
    va_start(ap, packet);

    puts("test6");

    while ((arg = va_arg(ap, char *))){
      if (arg != NULL){
       printf("%s\n", arg);
       strcat(info, arg);
      } else {
          break;
      }
    }
    printf("%s\n", info);
    puts("test7");
    va_end(ap);
*/
    int bytes;
    if((bytes=send(fd, "100", sizeof(packet),0)) < 0){
        perror("send");
        return 0;
    }
    
    return 1;
}

void setupSIG(){
    signal(SIGINT, SIGexit);
    signal(SIGQUIT, SIGexit);
}

void SIGexit(int sig){
    close(sock);
    exit(0);
}