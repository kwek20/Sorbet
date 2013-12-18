/* 
 * File:   pfcclient.c
 * Author: Bartjan Zondag & Kevin Rosendaal
 * 
 */

/*
 * Deze client kan een file uploaden/downloaden naar/van de PFC server.
 * Dit gebeurd via een synchronisatie protocol. 
 */

#include "pfc.h"

#define SERVER_PORT 2200
#define SERVER_NAME "localhost"
#define GET_REQUEST "GET / HTTP/1.0\r\n\r\n"

#define DEBUG_LEVEL 2

static void my_debug( void *ctx, int level, const char *str )
{
    if( level < DEBUG_LEVEL )
    {
        fprintf( (FILE *) ctx, "%s", str );
        fflush(  (FILE *) ctx  );
    }
}

struct sockaddr_in serv_addr;

int sluitVerbinding(int server_fd, x509_crt cacert, entropy_context entropy, ssl_context ssl);
int SendCredentials(ssl_context *ssl);
int InitCTX(int server_fd, ssl_context ssl, x509_crt cacert, int ret, const char *pers, entropy_context entropy, ctr_drbg_context ctr_drbg);

int main(int argc, char** argv) {

    // Usage: pfcclient /tmp/test.txt 192.168.1.1
    argc = 3;
    argv[1] = "test.txt";
    argv[2] = "127.0.0.1"; //moet nog veranderd worden
    
//    IS_CLIENT = MOOI;
    pfcClient(argv);
    
    /*
     * invoer bestand
     * invoer ip server
     */
    return (EXIT_SUCCESS);
}

/**
 * Hoofdprogramma voor de pfcClient
 * @param argv argumenten van commandline
 * @return 0 if succesvol. -1 if failed.
 */
int pfcClient(char** argv){
   
   
   ////////////////////////////////////////////////
   ////////////////////////////////////////////////
    int ret = 0, server_fd = -1;
    const char *pers = "ssl_client1";

    entropy_context entropy;
    ctr_drbg_context ctr_drbg;
    ssl_context ssl;
    x509_crt cacert;
     
    
    ////////////////////////////////////////
    ////////////////////////////////////////
   
   if((ServerGegevens(argv[2])) < 0){
       exit(EXIT_FAILURE);
   }
   
    
    printStart();
   //SSL_library_init();
   if(InitCTX(server_fd, ssl, cacert, ret, pers, entropy, ctr_drbg) != MOOI){exit(EXIT_FAILURE);}
   // Create socket
 //  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
 //      perror("Create socket error:");
 //      exit(EXIT_FAILURE);
 //  }
   
   
   //clients = (struct clientsinfo*) malloc(sizeof(struct clientsinfo));
 // if(SendCredentials(&ssl) != MOOI){exit(EXIT_FAILURE);}
  // if(ConnectNaarServer(&sockfd, ret, server_fd, ctr_drbg, entropy, ssl, cacert) != MOOI){exit(EXIT_FAILURE);}
   
//   ssl = SSL_new(ctx);      /* create new SSL connection state */
//   SSL_set_fd(ssl, sockfd);    /* attach the socket descriptor */
//   if (SSL_connect(ssl) == STUK){
//       ERR_print_errors_fp(stderr);
//       return STUK;
//   }
   
   if(SendCredentials(&ssl) != MOOI){exit(EXIT_FAILURE);}
//   if(ModifyCheckClient(&sockfd, argv[1]) < 0){
//       printf("error bij ModifyCheckClient\n");
//       exit(EXIT_FAILURE);
//   }
   exit(EXIT_FAILURE);
}

int InitCTX(int server_fd, ssl_context ssl, x509_crt cacert, int ret, const char *pers, entropy_context entropy, ctr_drbg_context ctr_drbg)
{
    /*
     * 0. Initialize the RNG and the session data
     */
    memset( &ssl, 0, sizeof( ssl_context ) );
    x509_crt_init( &cacert );

    printf( "\n  . Seeding the random number generator..." );
    fflush( stdout );

    entropy_init( &entropy );
    if( ( ret = ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        printf( " failed\n  ! ctr_drbg_init returned %d\n", ret );
        //goto exit;
    }

    printf( " ok\n" );

    /*
     * 0. Initialize certificates
     */
    printf( "  . Loading the CA root certificate ..." );
    fflush( stdout );

#if defined(POLARSSL_CERTS_C)
    ret = x509_crt_parse( &cacert, (const unsigned char *) test_ca_list,
                          strlen( test_ca_list ) );
#else
    ret = 1;
    printf("POLARSSL_CERTS_C not defined.");
#endif

    if( ret < 0 )
    {
        printf( " failed\n  !  x509_crt_parse returned -0x%x\n\n", -ret );
        //goto exit;
    }

    printf( " ok (%d skipped)\n", ret );

//   const SSL_METHOD *method;
//    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
//    SSL_load_error_strings();   /* Bring in and register error messages */
//    method = TLSv1_client_method();  /* Create new client-method instance */
//    ctx = SSL_CTX_new(method);   /* Create new context */
//    if ( ctx == NULL ){ERR_print_errors_fp(stderr);}

    if(ConnectNaarServer(ret, server_fd, ctr_drbg, entropy, ssl, cacert) != MOOI){exit(EXIT_FAILURE);}
   
    return MOOI;
}

/**
 * Functie serv_addr vullen
 * @param ip Ascii ip van server 
 * @return 0 if succesvol. -1 if failed.
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

/**
 * Functie maak verbinding met de server
 * @param sockfd file descriptor voor de server
 * @return 0 if succesvol. -1 if failed.
 */
int ConnectNaarServer(int ret, int server_fd, ctr_drbg_context ctr_drbg, entropy_context entropy, ssl_context ssl, x509_crt cacert){
    int* sockfd = 0;
    int len = 0;
    unsigned char buf[BUFFERSIZE];
    
    /*
     * 1. Start the connection
     */
    printf( "  . Connecting to tcp/%s/%4d...", SERVER_NAME,
                                               SERVER_PORT );
    fflush( stdout );

    if( ( ret = net_connect( &server_fd, SERVER_NAME,
                                         SERVER_PORT ) ) != 0 )
    {
        printf( " failed\n  ! net_connect returned %d\n\n", ret );
        return STUK;//sluitVerbinding(server_fd, cacert, entropy, ssl);
    }

    printf( " ok\n" );
    
    /*
     * 2. Setup stuff
     */
    printf( "  . Setting up the SSL/TLS structure..." );
    fflush( stdout );

    if( ( ret = ssl_init( &ssl ) ) != 0 )
    {
        printf( " failed\n  ! ssl_init returned %d\n\n", ret );
        return STUK;//sluitVerbinding(server_fd, cacert, entropy, ssl);
    }

    printf( " ok\n" );

    ssl_set_endpoint( &ssl, SSL_IS_CLIENT );
    ssl_set_authmode( &ssl, SSL_VERIFY_OPTIONAL );
    ssl_set_ca_chain( &ssl, &cacert, NULL, "PolarSSL Server 1" );

    ssl_set_rng( &ssl, ctr_drbg_random, &ctr_drbg );
    ssl_set_dbg( &ssl, my_debug, stdout );
    ssl_set_bio( &ssl, net_recv, &server_fd,
                       net_send, &server_fd );
    //ssl_set_max_version(&ssl, SSL_MAJOR_VERSION_3, SSL_MINOR_VERSION_1);
    
    /*
     * 4. Handshake
     */
    printf( "  . Performing the SSL/TLS handshake..." );
    fflush( stdout );

    while( ( ret = ssl_handshake( &ssl ) ) != 0 )
    {
        if( ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE )
        {
            printf( " failed\n  ! ssl_handshake returned -0x%x\n\n", -ret );
            return STUK;//goto exit;
        }
    }

    printf( " ok\n" );
    
    /*
     * 5. Verify the server certificate
     */
    printf( "  . Verifying peer X.509 certificate..." );

    if( ( ret = ssl_get_verify_result( &ssl ) ) != 0 )
    {
        printf( " failed\n" );

        if( ( ret & BADCERT_EXPIRED ) != 0 )
            printf( "  ! server certificate has expired\n" );

        if( ( ret & BADCERT_REVOKED ) != 0 )
            printf( "  ! server certificate has been revoked\n" );

        if( ( ret & BADCERT_CN_MISMATCH ) != 0 )
            printf( "  ! CN mismatch (expected CN=%s)\n", "PolarSSL Server 1" );

        if( ( ret & BADCERT_NOT_TRUSTED ) != 0 )
            printf( "  ! self-signed or not signed by a trusted CA\n" );

        printf( "\n" );
    }
    else
        printf( " ok\n" );

    /*
     * 3. Write the GET request
     */
    printf( "  > Write to server:" );
    fflush( stdout );

    len = sprintf( (char *) buf, GET_REQUEST );

    while( ( ret = ssl_write( &ssl, buf, len ) ) <= 0 )
    {
        if( ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE )
        {
            printf( " failed\n  ! ssl_write returned %d\n\n", ret );
            sluitVerbinding(server_fd, cacert, entropy, ssl);
        }
    }

    len = ret;
    printf( " %d bytes written\n\n%s", len, (char *) buf );

    /*
     * 7. Read the HTTP response
     */
    printf( "  < Read from server:" );
    fflush( stdout );

    do
    {
        len = sizeof( buf ) - 1;
        memset( buf, 0, sizeof( buf ) );
        ret = ssl_read( &ssl, buf, len );

        if( ret == POLARSSL_ERR_NET_WANT_READ || ret == POLARSSL_ERR_NET_WANT_WRITE )
            continue;

        if( ret == POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY )
            break;

        if( ret < 0 )
        {
            printf( "failed\n  ! ssl_read returned %d\n\n", ret );
            break;
        }

        if( ret == 0 )
        {
            printf( "\n\nEOF\n\n" );
            break;
        }

        len = ret;
        
        
        
        printf( " %d bytes read\n\n%s", len, (char *) buf );
         
    }
    
    while( 1 );
    printf("test 123456789");
    //if(SendCredentials(&ssl) != MOOI){exit(EXIT_FAILURE);}  //Sendcredentials
    ssl_close_notify( &ssl );
    
    ////////////////////////////////////
    ////////////////////////////////////
    
    if((connect(*sockfd,(struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0){
        perror("Connect error:");
        return STUK;
    }

    printf("Connectie succesvol\n");
    return MOOI;
}

/**
 * Functie verstuurt de modify-date van een bestand naar de server en
 * krijgt terug welke de nieuwste is.
 * @param sockfd socket waarop gecontroleerd moet worden
 * @param bestandsnaam bestandsnaam van bestand dat gecontroleerd moet worden
 * @return 0 if succesvol. -1 if failed.
 */
int ModifyCheckClient(ssl_context *ssl, char* bestandsnaam){
    struct stat bestandEigenschappen;
    stat(bestandsnaam, &bestandEigenschappen);
    
    char statusCode[4], seconden[40];
    char* buffer = malloc(BUFFERSIZE);
    int readCounter = 0;
    
    sprintf(seconden, "%i", (int) bestandEigenschappen.st_mtime);
    sprintf(statusCode, "%d", STATUS_MODCHK);
    sendPacket(ssl, STATUS_MODCHK, bestandsnaam, seconden, NULL);
    
    // Wacht op antwoord modifycheck van server
    if((readCounter = ssl_read(ssl, (unsigned char*)buffer, BUFFERSIZE)) <= 0) {
        //printf("%s(%i)\n", buffer, readCounter);
        perror("Receive modififycheck result error");
        return STUK;
    }
    sendPacket(ssl, STATUS_OK, NULL);
    switchResult(ssl, buffer);
    
    return MOOI;
}

/**
 * De credentials 
 * @param sockfd
 * @return 
 */
int SendCredentials(ssl_context *ssl){
    
    printf("Hello");
     /*
     * client verstuurd verzoek om in te loggen (302:username:password)
     * server verstuurd 404 als dit mag of 203 als dit niet mag
     */
    char *username, *password, *buffer;
    char *salt = "Sorbet";
    char hex[SHA256_DIGEST_LENGTH*2+1];
    int sR = 0; //switchResult

    
    for(;;){
        
        printf("\nUsername: ");
        buffer = getInput(50);
        username = malloc(strlen(buffer));
        strcpy(username,buffer);
        
        memset(buffer, 0 , strlen(buffer));
        
        printf("Password: ");
        buffer = getInput(50);
        
        password = malloc(strlen(buffer));
        strcpy(password,buffer);
        hashPassword(password, salt, hex);
        
        memset(buffer, 0 , strlen(buffer));
        
        printf("ready to send\n");
        sendPacket(ssl, STATUS_AUTH, username, hex, NULL);
        if((ssl_read(ssl, (unsigned char*)buffer, BUFFERSIZE)) < 0) {
            perror("Receive metadata OK error");
            return STUK;
        }

        sR = switchResult(ssl, buffer);
        switch(sR){
            case STUK: return STUK;
            case STATUS_AUTHFAIL: printf("Username or Password incorrect\n"); continue;
            case STATUS_AUTHOK: printf("Succesvol ingelogd\n"); return MOOI;
        }
    }
    
    return STUK;
}

int sluitVerbinding(int server_fd, x509_crt cacert, entropy_context entropy, ssl_context ssl)
{    
    net_close(server_fd);
    x509_crt_free(&cacert);
    entropy_free( &entropy );
    
    memset( &ssl, 0, sizeof( ssl ) );
    
//    x509_crt_free( &cacert );
//    net_close( server_fd );
//    ssl_free( &ssl );
//    entropy_free( &entropy );
//
//    memset( &ssl, 0, sizeof( ssl ) );
    
//#if defined(POLARSSL_SSL_CACHE_C)
//    ssl_cache_free(&cache);
//#endif
        
    return MOOI;
}
