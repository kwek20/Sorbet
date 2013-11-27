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
#include <semaphore.h>

void SIGexit(int sig);
void setupSIG();
void create(int *sock);
int transform(char *text, char** to);
int sendPacket(int sock, int packet, ...);
void getEOF(char *to);
struct sockaddr_in getServerAddr(int poort);

int cur_cli = 0;
sem_t client_wait; 
int sock;

/*
 * Main function, starts all threads
 */
int main(int argc, char** argv) {
    int poort = PORT;
    if (argc > 1 && argv[1] != NULL){
        poort = atoi(argv[1]);
    }

    setupSIG();

    //create semaphore
    if (sem_init(&client_wait, 0, 0) < 0){
        perror("semaphore");
        return -1;
    }
    //set it open
    sem_post(&client_wait);
    
    //make server data
    struct sockaddr_in server_addr = getServerAddr(poort);
    
    //make a socket
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
        //wait for free sem
        sem_wait(&client_wait);
        
        //extra check, just in case
        if(cur_cli < MAX_CLI){
            //create new thread for a new connection
            pthread_t client;
            pthread_create(&client, NULL, (void*)create, &sock);
            pthread_detach(client);
            cur_cli++;
            
            //can we still make clients? clear the sem again if possible
            if (cur_cli < MAX_CLI){
                sem_post(&client_wait);
            }
        }
    }
    return (EXIT_SUCCESS);
}

/**
 * Main function for every new thread
 * This will wait for a new client and act accordingly
 * @param sock the socket the server created
 */
void create(int *sock){
    //init vars
    int modus = 0, fd, rec;
    struct sockaddr_in client_addr;
    char buffer[BUFSIZ];
    
    //accept connection
    socklen_t size = sizeof(client_addr);
    if ((fd = accept(*sock, (struct sockaddr *)&client_addr, &size)) < 0){
         perror("accept");
    }

    //data for later usage
    char *ip;
    ip = inet_ntoa(client_addr.sin_addr);
    int poort = htons(client_addr.sin_port);

    //print information
    printf("-------------\nConnection accepted with client: %i\n", fd);
    printf("Ip nummer: %s\n", ip);
    printf("Port: %i\n", poort);
    printf("Ready to receive!\n");

    //variables used by things
    char **array = malloc(1);
    char *filename = malloc(1);
    int file = -1;

    //loop forever until client stops
    for ( ;; ){
        //receive info
        if ((rec = recv(fd, buffer, sizeof(buffer),0)) < 0){
            perror("recv");
            printf("Client error! Stopping... \n");
        } else if (rec == 0){
            //connection closed
        } else {
            //good
            
            //amount of parameters
            int values = 0;
            
            //check the current modus
            if (modus == 0){
                //was waiting for a modus, transform data to options
                values = transform(buffer, array);

                if (values < 1){
                    //?? empty, shouldnt have happened
                } else {
                    //set modus
                    modus = atoi(array[0]);
                    printf("Selected modus: %i\n", modus);
                    strcpy(filename, array[1]);

                    //send ack packet
                    sendPacket(fd, STATUS_OK, NULL);
                }
            } else {
                //hes in a modus? whatchu got for me
                if (rec < 50){
                    //want to stop?
                    char *eind = malloc(20);
                    getEOF(eind);

                    //is it an EOF packet?
                    if (strcmp(buffer, eind) == 0){
                        //stop data
                        puts("Stopping file transfer");
                        modus = 0;
                        close(file);
                        sendPacket(fd, STATUS_OK, NULL);
                        //continue;
                        
                        //values = 0;
                        break;
                    }
                }

                //was there a file still open?
                if (file < 0){
                    //no, open it
                    file = open(filename, O_WRONLY | O_CREAT, 0666);
                }
                
                //file open now?
                if (file != 0){
                    //ack that we received data
                    sendPacket(fd, STATUS_OK, NULL);
                    //MOAR data
                    //save it all
                    printf("Received %i bytes of data\n", rec);
                    write(file, buffer, rec);
                } else {
                    perror("open");
                }
                
                //clear received data
                memset(buffer, 0, sizeof(buffer));
            }
        }
    }

    printf("Client stopped\n");
    close(fd);
    cur_cli--;
    
    //open sem for new thread
    sem_post(&client_wait);
}

/**
 * Turn a string into an array of strings split by the identifier ":"
 * @param text the text to split
 * @param to a pointer to the array
 * @return the amount of splits done (amount of values)
 */
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

/**
 * Sends a packet to the fd defined
 * @param fd The fd to send this packet to
 * @param packet the packet number
 * @param ... the values for this packet, end with NULL
 * @return 1 if the packet was send succesfully. otherwise 0
 */
int sendPacket(int fd, int packet, ...){

    char *info = malloc(20);
    strcpy(info, "");

    char *p = malloc(20);
    sprintf(p, "%d", packet);

    strcat(info, p);
    strcat(info, ":");

    va_list ap;
    int i;
    
    va_start(ap, packet);
    
    for (i = packet; i != NULL; i = va_arg(ap, int)){
        printf("%d ", i);
    }
    va_end(ap);

    int bytes;
    if((bytes=send(fd, "100", sizeof(packet),0)) < 0){
        perror("send");
        return 0;
    }
    
    return 1;
}

/**
 * Sets the signals we catch
 */
void setupSIG(){
    signal(SIGINT, SIGexit);
    signal(SIGQUIT, SIGexit);
    signal(SIGSEGV, SIGexit);
}

/**
 * Our signal handler
 * @param sig the sig which ends
 */
void SIGexit(int sig){
    close(sock);
    exit(0);
}

/**
 * Creates the text needed for EOF packet
 * @param to where we place the text in
 */
void getEOF(char *to){
    strcpy(to, "");
    char *pID = malloc(20);
    sprintf(pID, "%d", 101);
    strcat(to, pID);
    strcat(to, ":EOF");
}

/**
 * creates a sockaddr_in with the needed information
 * @param poort the port used
 * @return the sockaddr_in we just made
 */
struct sockaddr_in getServerAddr(int poort){
    struct sockaddr_in server_addr;
    
    //clear data
    memset(&server_addr, '0' , sizeof(server_addr));

    //set family to ipv4, poort naar gegeven poort, en luister naar elk addres
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(poort);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    return server_addr;
}