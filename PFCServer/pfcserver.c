/* 
 * File:   pfcserver.c
 * Author: Brord van Wierst
 *
 * Created on November 19, 2013, 10:37 AM
 */

#include "../PFCClient/pfc.h"
#include "pfc.h"
#include <memory.h>
#include <pthread.h>
#include <stdarg.h>
#include <semaphore.h>

void setupSIG();
void SIGexit(int sig);

void quit();
void stopClient(int fd);

void create(int *sock);
struct sockaddr_in getServerAddr(int poort);

void command(void);
int numcli(char **args, int amount);
int help(char **args, int amount);
int clientinfo(char **args, int amount);
int initDatabase(char **args, int amount);
int printTable(char **args, int amount);

int printClientInfo(struct sockaddr_in client, int number);

int ReceiveCredentials(char* username, char* password);

const static struct {
    const char *name;
    int (*func)(char **args, int amount);
} function_map[] = {
    {"help", help},{"info", help},
    {"numcli", numcli},
    {"clientinfo", clientinfo},
    {"initdb", initDatabase},
    {"printdb", printTable},
    {"adduser", createUser},
    {"createuser", createUser},
    {"removeuser", removeUser},
};

int sock, bestandfd, cur_cli = 0;
sem_t client_wait; 

struct sockaddr_in *clients; 


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
    clients = (struct sockaddr_in*) malloc(MAX_CLI*sizeof(struct sockaddr_in));
    
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
    int result = 0, fd, rec, i, temp = 0;
    struct sockaddr_in client_addr;
    char buffer[BUFFERSIZE];
    char** to = malloc(1);
    
    //open sem for new thread
    sem_post(&client_wait);
    
    //accept connection
    socklen_t size = sizeof(client_addr);
    if ((fd = accept(*sock, (struct sockaddr *)&client_addr, &size)) < 0){
         perror("accept");
    }

    //data for later usage
    char *ip;
    ip = inet_ntoa(client_addr.sin_addr);
    int poort = htons(client_addr.sin_port);

    clients[fd-4] = client_addr;
    
    //print information
    printf("\n-------------\nConnection accepted with client: %i\n", fd);
    printf("IP Address: %s\n", ip);
    printf("Port: %i\n", poort);
    printf("Ready to receive!\n");
    
    //Login moet nog naar een functie
    for (i = 0; i < LOGINATTEMPTS; i++){
        printf("at login\n");
        bzero(buffer, BUFFERSIZE);
        if(recv(fd, buffer, BUFFERSIZE, 0) < 0){
            perror("recv error");
            return;
        }
        transform(buffer, to);
        printf("to[0]: %s, to[1]: %s - to[2]: %s\n",to[0], to[1],to[2]);
        if((temp = ReceiveCredentials(to[1], to[2])) == MOOI){
            sendPacket(fd, STATUS_AUTHOK);
            break;
        }
        printf("recvCred: %i\n",temp);
        if(i < 2){
            sendPacket(fd, STATUS_AUTHFAIL);
        }else{
            sendPacket(fd, STATUS_CNA);
            stopClient(fd);
            return;
        }
    }
    //End of Login
    
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
    stopClient(fd);
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
    closeDB();
    close(sock);
    exit(MOOI);
}

void stopClient(int fd){
    printf("Client stopped\n");
    memset(&clients[fd-4], 0, sizeof(struct sockaddr_in));
    close(fd);
    cur_cli--;
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
        printf(" > ");
        command = getInput(50);
        amount = transformWith(command, args, " ");
        if (amount < 1) continue;
        if((strcasecmp(args[0], "exit") == MOOI) || (strcasecmp(args[0], "stop") == MOOI) || (strcasecmp(args[0], "quit") == MOOI)){
            break;
        }
        
        int i;
        for (i = 0; i < (sizeof(function_map) / sizeof(function_map[0])); i++){
            if (!strcasecmp(function_map[i].name, args[0]) && function_map[i].func){
                function_map[i].func(args, amount);
                i = -1;
                break;
            }
        }
        if (i != -1) help(args, amount);
    }
    
    quit();
}

int clientinfo(char **args, int amount){
    //data for later usage
    int i, j = 0;
    if (amount > 1){
        i = atoi(args[1]);
        if (clients[i].sin_port != 0){
            printClientInfo(clients[i], i);
        }
    }
    
    for (i=0; i<MAX_CLI; i++){
        if (clients[i].sin_port != 0){
            j = 1;
            printClientInfo(clients[i], i+1);
        }
    }
    
    if (j == 0){
        printf("No clients connected\n");
    }
    return MOOI;
}

int printClientInfo(struct sockaddr_in client, int number){
    if (client.sin_port != 0){
        char *ip;
        ip = inet_ntoa(client.sin_addr);

        printf("Info from client %i\n", number);
        printf("- IP Address: %s\n", ip);
        printf("- Port: %i\n", htons(client.sin_port));
        
        return MOOI;
    }
    return STUK;
}

int help(char **args, int amount){
    if (amount > 1){
        if (strcasecmp(args[1], "help") == MOOI){
            printf("help [command] -> Show help about a specific command\n");
            printf("help -> Show all the commands in a list\n");
        } else if (strcasecmp(args[1], "numcli") == MOOI){
            printf("numcli -> Will show you the current amount of online clients\n");
        } else if (strcasecmp(args[1], "exit") == MOOI || strcasecmp(args[1], "stop") == MOOI || strcasecmp(args[1], "quit") == MOOI){
            printf("%s -> This will gracefully stop the server and it's active connections\n", args[1]);
            printf("aliases -> quit, stop, exit\n");
        }
    } else {
        char *options = malloc(50);
        strcpy(options, "exit, ");
        int i;
        for (i = 0; i < (sizeof(function_map) / sizeof(function_map[0])); i++){
            strcat(options, function_map[i].name);
            strcat(options, ", ");
        }
        options[strlen(options)-2] = '\0';
        printf("Available commands: %s\n", options);
    }
    return MOOI;
}

int numcli(char **args, int amount){
    printf("Currently available threads: %i.\n", cur_cli);
    return MOOI;
}

int initDatabase(char **args, int amount){
    connectDB();
    return MOOI;
}

int printTable(char **args, int amount){
    if (amount < 2){
        return STUK;
    }
    
    char* sql = malloc(100);
    strcpy(sql, "SELECT * FROM ");
    strcat(sql, args[1]);
    strcat(sql, ";");
   
    printRes(selectQuery(sql));
    return MOOI;
}

/**
 * Deze functie zal controleren of de gebruikersnaam en het wachtwoord van de user kloppen
 * @param sockfd
 * @param username
 * @param password
 * @return 
 */

int ReceiveCredentials(char* username, char* password){
    int temp = 0;
    if((temp = strcmp(username,"test")) != 0){
        printf("username fail temp: %i\n",temp);
        return STUK;
    }else{
        if((temp = strcmp(password,"1234")) != 0){
            printf("password fail temp: %i\n",temp);
            return STUK;
        }
    }
    printf("username and password OK\n");
    return MOOI;
}