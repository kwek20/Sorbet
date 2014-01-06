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

//Server Only
char* getPassWord(char *name);
int checkCredentials();
int sendCredentials();

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

int printClientInfo(struct clientsinfo client, int number);

int ReceiveCredentials(char* username, char* password);

const static struct {
    const char *name;
    int (*func)(char **args, int amount);
    int aliasesAmount;
    char *description;
    const char * const *aliases;
} function_map[] = {
    {"help", help, 1, "help [command] -> Show help about a specific command\n help -> Show all the commands in a list", (const char * const []){"info"}},
    {"numcli", numcli, 0, "Will show you the current amount of online clients", (const char * const []){}},
    {"clientinfo", clientinfo, 1, "Shows the info form the client defined, or all clients", (const char * const []){"cinfo"}},
    {"initdb", initDatabase, 0, "Initializes the database if it wasnt yet for some reason", (const char * const []){}},
    {"printtable", printTable, 1, "Prints all the data in the table defined", (const char * const []){"showtable"}},
    {"adduser", createUser, 1, "Creates a user with the name and password", (const char * const []){"createuser"}},
    {"removeuser", removeUser, 0, "Removes the defined user", (const char * const []){}},
    {"quit", help, 3, "This will gracefully stop the server and it's active connections", (const char * const []){"exit", "stop", "end"}}
};

int sock, bestandfd, cur_cli = 0;
sem_t client_wait; 

/*
 * Main function, starts all threads
 */
int main(int argc, char** argv) {
    int poort = NETWERKPOORT;
    if (argc > 1 && argv[1] != NULL){
        poort = atoi(argv[1]);
        if (poort == 0) poort = NETWERKPOORT;
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
    clients = (struct clientsinfo*) malloc(MAX_CLI*sizeof(struct clientsinfo));
    
    //make a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket");
        return -1;
    }
    
    int setsock = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &setsock, sizeof(int))){
        perror("Socket setting");
        return STUK;
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
    connectDB();
    
    IS_CLIENT = STUK;
    mkdir("userfolders", S_IRWXU);
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

    //add to the list
    clients[fd-4].client = client_addr;
    
    //print information
    printf("\n-------------\nConnection accepted with client: %i\n", fd);
    printf("IP Address: %s\n", ip);
    printf("Port: %i\n", poort);
    printf("Waiting for authentication...\n");
    
    //Login moet nog naar een functie
    for (i = 0; i < LOGINATTEMPTS; i++){
        bzero(buffer, BUFFERSIZE);
        int bytes = 0;
        if((bytes = recv(fd, buffer, BUFFERSIZE, 0)) <= 0){
            perror("recv error");
            stopClient(fd);
            return;
        }
        char** to = malloc(BUFFERSIZE + 100);
        bzero(to, BUFFERSIZE + 100);
        transform(buffer, to);
        if((temp = ReceiveCredentials(to[1], to[2])) == MOOI){
            sendPacket(fd, STATUS_AUTHOK, NULL);
            //add username
            clients[fd-4].username = malloc(strlen(to[1]));
            strcpy(clients[fd-4].username, to[1]);
            
            char *folder = malloc(sizeof(to[1]) + strlen("userfolders/"));
            strcpy(folder, "userfolders/");
            strcat(folder, to[1]);
            
            mkdir(folder, S_IRWXU);
            break;
        }
        
        if (to){
            free(*to);
            to = NULL;
        }
        
        if(i < 2){
            sendPacket(fd, STATUS_AUTHFAIL, NULL);
            printf("Username/PW combinatie fout. Aantal pogingen: %i\n",i+1);
        } else {
            sendPacket(fd, STATUS_CNA, NULL);
            stopClient(fd);
            return;
        }
    }
    
    bzero(buffer, BUFFERSIZE);
    
    //End of Login
    printf("user %s has logged in, awaiting orders.\n", clients[fd-4].username);
    //loop forever until client stops
    for ( ;; ){
        //receive info
        if ((rec = recv(fd, buffer, BUFFERSIZE,0)) < 0){
            perror("recv");
            printf("Client error! Stopping... \n");
            break;
        } else if (rec == 0){
            //connection closed
            printf("Client closed connection! Stopping... \n");
            break;
        } else {
            //good
            if ((result = switchResult(&fd, buffer)) == STUK){
                //error
                break;
            }
        }
        bzero(buffer, BUFFERSIZE);
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
    close(sock);
    closeDB();
    exit(MOOI);
}

void stopClient(int fd){
    printf("Client stopped\n");
    bzero(&clients[fd-4], sizeof(clients[fd-4]));
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
    char **args = malloc(51);
    
    int i, j, func_ret = MOOI;
    for ( ;; ){
        printf(" > ");
        command = getInput(50);
        
        amount = transformWith(command, args, " ");
        
        if (amount < 1) continue;
        if (strcasecmp(args[0], "stop") == 0 || strcasecmp(args[0], "exit")  == 0  || strcasecmp(args[0], "quit") == 0  || strcasecmp(args[0], "end") == 0 ) break;
        
        for (i = 0; i < (sizeof(function_map) / sizeof(function_map[0])); i++){
            if (!strcasecmp(function_map[i].name, args[0]) && function_map[i].func){
                func_ret = function_map[i].func(args, amount);
                i = -1;
                break;
            } else {
                for (j=0; j<function_map[i].aliasesAmount; j++){
                    if (!strcasecmp(function_map[i].aliases[j], args[0]) && function_map[i].func){
                        func_ret = function_map[i].func(args, amount);
                        i = -1;
                        goto end_loop;
                    }
                }
                
            }
        }
        end_loop:
        
        if (i != STUK ){ 
            help(args, amount);
        } else if (func_ret == STUK) {
            char **newargs = malloc((sizeof(args)+1)*sizeof(args[0]));
            memcpy(newargs+1, args, (sizeof(args)+1)*sizeof(args[0]));
            newargs[0] = "help";
            help(newargs, amount+1);
        }
        
        memset(command, 0, 50);
        memset(args, 0, 51);
        
        
    }
    
    if (args){
        free(*args);
        args = NULL;
    }
    
    quit();
}

int clientinfo(char **args, int amount){
    //data for later usage
    int i, j = 0;
    if (amount > 1){
        i = atoi(args[1]);
        if (clients[i].client.sin_port != 0){
            printClientInfo(clients[i], i);
        }
    }
    
    for (i=0; i<MAX_CLI; i++){
        if (clients[i].client.sin_port != 0){
            j = 1;
            printClientInfo(clients[i], i+1);
        }
    }
    
    if (j == 0){
        printf("No clients connected\n");
    }
    return MOOI;
}

int printClientInfo(struct clientsinfo client, int number){
    if (client.client.sin_port != 0){
        char *ip;
        ip = inet_ntoa(client.client.sin_addr);

        printf("Info from client %s\n", client.username);
        printf("- IP Address: %s\n", ip);
        printf("- Port: %i\n", htons(client.client.sin_port));
        return MOOI;
    }
    return STUK;
}

int help(char **args, int amount){
    int i, ret = STUK;
    if (amount > 1){
        int j;
        for (i = 0; i < (sizeof(function_map) / sizeof(function_map[0])); i++){
            if (!strcasecmp(function_map[i].name, args[1]) && function_map[i].func){
                printf(" %s -> %s\n", args[1], function_map[i].description);
                for (j=0; j < function_map[i].aliasesAmount; j++){
                    printf(" Alias: %s\n", function_map[i].aliases[j]);
                }
                ret = MOOI;
                break;
            }
        }
    } else {
        char *options = malloc(100);
        strcpy(options, "exit, ");
        for (i = 0; i < (sizeof(function_map) / sizeof(function_map[0])); i++){
            strcat(options, function_map[i].name);
            strcat(options, ", ");
        }
        options[strlen(options)-2] = '\0';
        printf("Available commands: %s\n", options);
        
        free(options);
    }
    return ret;
}

int numcli(char **args, int amount){
    printf("Currently available threads: %i.\n", cur_cli);
    return MOOI;
}

int initDatabase(char **args, int amount){
    return connectDB();
}

int printTable(char **args, int amount){
    if (amount < 2){
        return STUK;
    }
    
    char* sql = malloc(15);
    strcpy(sql, "SELECT * FROM ");
    strcat(sql, args[1]);
    strcat(sql, ";");
    
    sqlite3_stmt *res = selectQuery(sql);
    if (res == NULL){
        return STUK;
    }
    printRes(res);
    sqlite3_finalize(res);
    free(sql);
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
    puts("1");
    char saltedPassword[SHA256_DIGEST_LENGTH*2];
    puts("2");
    char *salt = getSalt(username);
    puts("3");
    
    hashPassword(password, salt, saltedPassword);
    puts("4");
    
    
    if(strcmp(saltedPassword, getPassWord(username)) != 0){
        puts("4.5");
        return STUK;
    }
    puts("5");
    return MOOI;
}

