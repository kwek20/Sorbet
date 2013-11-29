/* 
 * File:   pfcserver.c
 * Author: Brord van Wierst
 *
 * Created on November 19, 2013, 10:37 AM
 */

#include "pfc.h"

#include <memory.h>
#include <pthread.h>
#include <stdarg.h>
#include <semaphore.h>

void setupSIG();
void SIGexit(int sig);
void quit();

void create(int *sock);
struct sockaddr_in getServerAddr(int poort);

void command(void);
void help(char **args, int amount);
void numcli();

int sock, bestandfd, cur_cli = 0;
sem_t client_wait; 

/*
 * Main function, starts all threads
 */
int main(int argc, char** argv) {
    int poort = NETWERKPOORT;
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
    
    printStart();
    
    pthread_t cmd;
    pthread_create(&cmd, NULL, (void*)command, NULL);
    
    printf("Private file cloud server ready!\nWe're listening for clients on port \"%i\".\n", NETWERKPOORT);
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
    
    pthread_join(cmd, NULL);
    return (EXIT_SUCCESS);
}

/**
 * Main function for every new thread
 * This will wait for a new client and act accordingly
 * @param sock the socket the server created
 */
void create(int *sock){
    //init vars
    int result = 0, fd, rec;
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
    printf("\n-------------\nConnection accepted with client: %i\n", fd);
    printf("IP Address: %s\n", ip);
    printf("Port: %i\n", poort);
    printf("Ready to receive!\n");

    //loop forever until client stops
    for ( ;; ){
        //receive info
        if ((rec = recv(fd, buffer, sizeof(buffer),0)) < 0){
            perror("recv");
            printf("Client error! Stopping... \n");
            break;
        } else if (rec == 0){
            //connection closed
            printf("Client closed connection! Stopping... \n");
            break;
        } else {
            //good
            if ((result = switchResult(&fd, buffer)) < 0){
                //error
                break;
            } else {
                //wooo
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
    quit();
}

void quit(){
    puts("\nStopping server.....");
    close(sock);
    exit(MOOI);
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

/******************************************************
 *                  COMMANDS FROM HERE
 ******************************************************/

void command(void){
    int amount;
    char *command;
    char **args = malloc(1);
    for ( ;; ){
        printf(" >");
        command = getInput(50);
        amount = transformWith(command, args, " ");
        if (amount < 1) continue;
        if (strcasecmp(args[0], "help") == MOOI){
            help(args, amount);
        } else if(strcasecmp(args[0], "numcli") == MOOI){
            numcli();
        } else if(strcasecmp(args[0], "exit") == MOOI){
            break;
        } else {
            help(args, amount);
        }
    }
    
    quit();
}

void help(char **args, int amount){
    if (amount > 1){
        if (strcasecmp(args[1], "help") == MOOI){
            printf("help [command] -> Show help about a specific command\n");
            printf("help -> Show all the commands in a list\n");
        } else if (strcasecmp(args[1], "numcli") == MOOI){
            printf("numcli -> Will show you the current amount of online clients\n");
        } else if (strcasecmp(args[1], "exit") == MOOI){
            printf("exit -> This will gracefully stop the server and it's active connections\n");
        }
    } else {
        printf("Available commands: help, numcli, exit\n");
    }
}

void numcli(){
    printf("Currently available threads: %i.\n", cur_cli);
}