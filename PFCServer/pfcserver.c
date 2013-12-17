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

#define CERTIFICATE "sorbet.pem"
#define DEBUG_LEVEL 5
#define HTTP_RESPONSE \
    "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n" \
    "<h2>PolarSSL Test Server</h2>\r\n" \
    "<p>Successful connection using: %s</p>\r\n"

void setupSIG();
void SIGexit(int sig);

void quit();
void stopClient(fdinfo *fds);

void create(int *listen_fd);
struct sockaddr_in getServerAddr(int poort);

void command(void);
int numcli(char **args, int amount);
int help(char **args, int amount);
int clientinfo(char **args, int amount);
int initDatabase(char **args, int amount);
int printTable(char **args, int amount);

int printClientInfo(struct clientsinfo client, int number);

int ReceiveCredentials(char* username, char* password);
int LoadCertificates(char* CertFile, char* KeyFile);

int sluitVerbinding(int client_fd, x509_crt srvcert, pk_context pkey);

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

int ret, bestandfd, cur_cli = 0;
sem_t client_wait; 
x509_crt srvcert;
pk_context pkey;
entropy_context entropy;
ctr_drbg_context ctr_drbg;
int client_fd;
int listen_fd;
const char *pers = "ssl_server";

static void my_debug( void *ctx, int level, const char *str )
{
    if( level < DEBUG_LEVEL )
    {
        fprintf( (FILE *) ctx, "%s", str );
        fflush(  (FILE *) ctx  );
    }
}

/*
 * Main function, starts all threads
 */
int main(int argc, char** argv) {
    int poort = NETWERKPOORT;
    
    if (argc > 1 && argv[1] != NULL){
        poort = atoi(argv[1]);
    }

    setupSIG();
    
    /*
     * 1. Load the certificates and private RSA key
     */
    printf( "\n  . Loading the server cert. and key..." );
    fflush( stdout );

    x509_crt_init( &srvcert );

    /*
     * This demonstration program uses embedded test certificates.
     * Instead, you may want to use x509_crt_parse_file() to read the
     * server and CA certificates, as well as pk_parse_keyfile().
     */
    ret = x509_crt_parse( &srvcert, (const unsigned char *) test_srv_crt,
                          strlen( test_srv_crt ) );
    if( ret != 0 )
    {
        printf( " failed\n  !  x509_crt_parse returned %d\n\n", ret );
        sluitVerbinding(client_fd, srvcert, pkey);
    }

    ret = x509_crt_parse( &srvcert, (const unsigned char *) test_ca_list,
                          strlen( test_ca_list ) );
    if( ret != 0 )
    {
        printf( " failed\n  !  x509_crt_parse returned %d\n\n", ret );
        sluitVerbinding(client_fd, srvcert, pkey);
    }

    pk_init( &pkey );
    ret =  pk_parse_key( &pkey, (const unsigned char *) test_srv_key,
                         strlen( test_srv_key ), NULL, 0 );
    if( ret != 0 )
    {
        printf( " failed\n  !  pk_parse_key returned %d\n\n", ret );
        sluitVerbinding(client_fd, srvcert, pkey);
    }

    printf( " ok\n" );

    /*
     * 2. Setup the listening TCP socket
     */
    printf( "  . Bind on https://localhost:4433/ ..." );
    fflush( stdout );

    if( ( ret = net_bind( &listen_fd, NULL, poort ) ) != 0 )
    {
        printf( " failed\n  ! net_bind returned %d\n\n", ret );
        sluitVerbinding(client_fd, srvcert, pkey);
    }

    printf( " ok\n" );

    /*
     * 3. Seed the RNG
     */
    printf( "  . Seeding the random number generator..." );
    fflush( stdout );

    entropy_init( &entropy );
    if( ( ret = ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        printf( " failed\n  ! ctr_drbg_init returned %d\n", ret );
        sluitVerbinding(client_fd, srvcert, pkey);
    }

    printf( " ok\n" );
    
    //create semaphore
    if(sem_init(&client_wait, 0, 0) < 0){
        perror("semaphore");
        return STUK;
    }
    //set it open
    sem_post(&client_wait);
    
    //make server data
    //struct sockaddr_in server_addr = getServerAddr(poort);
    //clients = (struct clientsinfo*) malloc(MAX_CLI*sizeof(struct clientsinfo));
    
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
            pthread_create(&client, NULL, (void*)create, &listen_fd);
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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int sluitVerbinding(int client_fd, x509_crt srvcert, pk_context pkey)
{    
    net_close(client_fd);
    x509_crt_free(&srvcert);
    pk_free(&pkey);
    
//#if defined(POLARSSL_SSL_CACHE_C)
//    ssl_cache_free(&cache);
//#endif
        
    return MOOI;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * Main function for every new thread
 * This will wait for a new client and act accordingly
 * @param listen_fd the socket the server created
 */
void create(int *listen_fd){
    //init vars
    fdinfo *fds = malloc(sizeof(fdinfo));
    int fd, i, temp = 0;
    struct sockaddr_in client_addr;
    unsigned char buffer[BUFFERSIZE];
    char** to;
    //SSL *ssl;
    ssl_context ssl;
    
#if defined(POLARSSL_SSL_CACHE_C)
    ssl_cache_context cache;
#endif

    //open sem for new thread
    sem_post(&client_wait);
    
    //////////////////////////////////////////////////////////
    
    /*
     * 4. Setup stuff
     */
    printf( "  . Setting up the SSL data...." );
    fflush( stdout );

    if( ( ret = ssl_init( &ssl ) ) != 0 )
    {
        printf( " failed\n  ! ssl_init returned %d\n\n", ret );
        sluitVerbinding(client_fd, srvcert, pkey);
    }

    ssl_set_endpoint( &ssl, SSL_IS_SERVER );
    ssl_set_authmode( &ssl, SSL_VERIFY_NONE );

    ssl_set_rng( &ssl, ctr_drbg_random, &ctr_drbg );
    ssl_set_dbg( &ssl, my_debug, stdout );

#if defined(POLARSSL_SSL_CACHE_C)
    ssl_set_session_cache( &ssl, ssl_cache_get, &cache,
                                 ssl_cache_set, &cache );
#endif

    ssl_set_ca_chain( &ssl, srvcert.next, NULL, NULL );
    ssl_set_own_cert( &ssl, &srvcert, &pkey );

    printf( " ok\n" );

reset:
#ifdef POLARSSL_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        polarssl_strerror( ret, error_buf, 100 );
        printf("Last error was: %d - %s\n\n", ret, error_buf );
    }
#endif

    if( client_fd != -1 )
        net_close( client_fd );

    ssl_session_reset( &ssl );

    /*
     * 3. Wait until a client connects
     */
    client_fd = -1;

    printf( "  . Waiting for a remote connection ..." );
    fflush( stdout );

    if( ( ret = net_accept( *listen_fd, &client_fd, NULL ) ) != 0 )
    {
        printf( " failed\n  ! net_accept returned %d\n\n", ret );
        sluitVerbinding(client_fd, srvcert, pkey);
    }

    ssl_set_bio( &ssl, net_recv, &client_fd,
                       net_send, &client_fd );
    
    
    fds->clientfd = &client_fd;
    fds->ssl = &ssl;

    printf( " ok\n" );
    
    /*
     * 5. Handshake
     */
    printf( "  . Performing the SSL/TLS handshake..." );
    fflush( stdout );

    while( ( ret = ssl_handshake( &ssl ) ) != 0 )
    {
        if( ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE )
        {
            printf( " failed\n  ! ssl_handshake returned %d\n\n", ret );
            goto reset;
        }
    }

    printf( " ok\n" );
    
    ////////////////////////////////////////////////////
    
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
        if((to = malloc(BUFFERSIZE)) < 0){
            perror("recv error");
            return;
        }
        printf("voor SSLREAD login\n");
        ssl_read(&ssl, buffer, BUFFERSIZE);
        //err_print_errors_fp(stderr);
        transform((char*)buffer, to);
        if((temp = ReceiveCredentials(to[1], to[2])) == MOOI){
            sendPacket(&ssl, STATUS_AUTHOK, NULL);
            //add username
            clients[fd-4].username = malloc(strlen(to[1]));
            strcpy(clients[fd-4].username, to[1]);
            
            char *folder = malloc(strlen(to[1]) + strlen("userfolders/"));
            strcpy(folder, "userfolders/");
            strcat(folder, to[1]);
            
            mkdir(folder, S_IRWXU);
            break;
        }
        
        if(i < 2){
            sendPacket(&ssl, STATUS_AUTHFAIL, NULL);
        } else {
            sendPacket(&ssl, STATUS_CNA, NULL);
            stopClient(fds);
            return;
        }
    }
    
    bzero(buffer, BUFFERSIZE);
    
    //End of Login
    printf("user %s has logged in, awaiting orders.\n", clients[fd-4].username);
    //loop forever until client stops
    /* 
     * Not in use for SSL implementation
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
            if ((result = switchResult(ssl, buffer)) == STUK){
                //error
                break;
            } else {
                //wooo
            }
        }
        bzero(buffer, BUFFERSIZE);
    }
    */
    stopClient(fds);
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
    close(listen_fd);
    closeDB();
    exit(MOOI);
}

void stopClient(fdinfo *fds){
    int *fd = fds->clientfd;
    
    printf("Client stopped\n");
    memset(&clients[*fd-4], 0, sizeof(struct sockaddr_in));
    sem_post(&client_wait);
    ssl_free(fds->ssl);
    close(*fd);
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
            memcpy(newargs+1, args, sizeof(args));
            newargs[0] = "help";
            help(newargs, amount+1);
        }
        
        memset(command, 0, 50);
        memset(args, 0, 51);
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
    char saltedPassword[SHA256_DIGEST_LENGTH*2];
    char *salt = getSalt(username);
    
    hashPassword(password, salt, saltedPassword);
    
    
    
    if(strcmp(saltedPassword, getPassWord(username)) != 0){
        printf("password fail temp: %i\n",temp);
        return STUK;
    }
    return MOOI;
}
