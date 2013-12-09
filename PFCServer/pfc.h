/*
 * File:   pfc.h
 * Author: Bartjan Zondag & Kevin Rosendaal & Brord van Wierst
 *
 * Created on November 27, 2013, 11:26 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <locale.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sqlite3.h>
#include <openssl/sha.h>

#define BUFFERSIZE 4096
#define NETWERKPOORT 2200

#define STUK -1
#define MOOI 0

#define MAX_CLI 10

// 1xx OK en gerelateerde statussen
#define STATUS_OK  100 // OK (bevestigingscode)
#define STATUS_EOF 101 // End-of-file.
// 2xx Errors en gerelateerde statussen
#define STATUS_CL  200 // Verbinding verbroken.
#define STATUS_FC  201 // Bestand corrupt.
#define STATUS_WPN 202 // Verkeerde packet nummer.
#define STATUS_CNA 203 // Verbinding niet toegestaan.
// 3xx Client naar server requests
#define STATUS_CR  300 // Client vraagt bestandsoverdracht aan.
#define STATUS_MODCHK 301 // Client vraagt aan de server of er nieuwe/gewijzigde bestanden zijn.
#define STATUS_AUTH 302 //Client stuurt credentials naar server.
// 4xx Server naar client requests
#define STATUS_OLD 401 //Server geeft aan dat file op server ouder is.
#define STATUS_NEW 402 //Server geeft aan dat file op server nieuwer is.
#define STATUS_SAME 403 //Server geeft aan dat de file hetzelfde is als die op de server.
#define STATUS_AUTHOK 404 //Server verteld de client dat de authenticatie gelukt is.
#define STATUS_AUTHFAIL 405 //Server verteld de client dat de authenticatie is mislukt.

#define LOGINATTEMPTS 3 //Binnen dit aantal moet de gebruiker het wachtwoord goed raden. Anders wordt de verbinding verbroken.

typedef struct clientsinfo{
    struct sockaddr_in client;
    char* username;
} clientsinfo;

clientsinfo *clients;
int IS_CLIENT;

int pfcClient(char** argv);
int ServerGegevens(char* ip);
int BestaatDeFile(char* fileName);
int ConnectNaarServer(int* sockfd);
int FileTransferSend(int* sockfd, char* bestandsnaam);
int FileTransferReceive(int* sockfd, char* bestandsnaam, int time);
int OpenBestand(char* bestandsnaam);
int ModifyCheckServer(int* sockfd, char* bestandsnaam, char* timeleft);
int ModifyCheckClient(int* sockfd, char* bestandsnaam);

int transform(char *text, char** to);
int transformWith(char *text, char** to, char *delimit);

int switchResult(int* sockfd, char* buffer);
int sendPacket(int fd, int packet, ...);
int waitForOk(int sockfd);

int changeModTime(char *bestandsnaam, int time);
int modifiedTime(char *bestandsnaam);

char *toString(int number);
void getEOF(char *to);
void printStart(void);
char* getInput(int max);
void printArray(int length, char *array[]);

int hashPassword(char *password, char *salt, char to[]);
int randomSalt(char *salt, int aantalBytes);
int convertHashToString(char *stringHash, unsigned char hash[]);

int ConnectRefused(int* sockfd);

//DB - Will be edited
int callback(void *NotUsed, int argc, char **argv, char **azColName);
int connectDB();
int initDB();

void printRes(sqlite3_stmt *res);
sqlite3_stmt* selectQuery(char *query);

int closeDB();

int createUser(char **args, int amount);
int removeUser(char **args, int amount);
int userExists(char* name);
char* getPassWord(char *name);

int checkCredentials();
int sendCredentials();
int writePasswordToLocalDB(int rc);
