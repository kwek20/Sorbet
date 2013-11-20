/* 
 * File:   pfcserver.c
 * Author: FPC project group
 *
 * Created on November 19, 2013, 10:37 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <memory.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>


#define PORT 2200
#define MAX_CLI 10

void SIGexit(int sig);
void setupSIG();
void create(int *sock);
int transform(char *text, char** to);
void sendPacket(int sock, int pID);

int cur_cli = 0;

/*
 * 
 */
int main(int argc, char** argv) {
    int poort = PORT;
    if (argc > 1 && argv[1] != NULL){
        poort = atoi(argv[1]);
    }

    setupSIG();

    struct sockaddr_in server_addr, client_addr;

    //clear data
    memset(&server_addr, '0' , sizeof(server_addr));

    //set family to ipv4, poort naar gegeven poort, en luister naar elk addres
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(poort);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int sock = 0;
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
            printf("Created client %i\n", cur_cli);
            pthread_t client;
            pthread_create(&client, NULL, (void*)create, &sock);
            pthread_detach(client);
        }
    }

    return (EXIT_SUCCESS);
}

void create(int *sock){
    printf("client on sock %i ready! \n", *sock);

    int modus, fd, rec;
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

    for ( ;; ){
        if ((rec = recv(fd, buffer, sizeof(buffer),0)) < 0){
            perror("recv");
            printf("Client error! Stopping... \n");
        } else if (rec == 0){
            //connection closed
        } else {
            //good
            if (modus == 0){
                char **array;
                int values;
                values = transform(buffer, array);

                if (values < 1){
                    //?? empty, shouldnt have happened
                } else{
                    modus = atoi(array[0]);
                    printf("%i\n", modus);
                }
                //modus = atoi();
            }
        }
    }

    cur_cli--;
}

int transform(char *text, char** to){
    char *options[10];
    char *temp;

    temp = strtok(text, ":");
    int colon = 0;

    while (temp != NULL || temp != '\0'){
        options[colon] = temp;
        temp = strtok(NULL, ":");
        colon++;
    }
    options[colon] = 0;
    to = options;
    return colon-1;
}

void sendPacket(int sock, int pID){

}

void setupSIG(){
    signal(SIGINT, SIGexit);
    signal(SIGQUIT, SIGexit);
}

void SIGexit(int sig){

}