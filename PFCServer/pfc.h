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
#include <fts.h>
#include <math.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define CERTIFICATE "Sorbet.pem"

//Server/Client Defines needed for general options
#define BUFFERSIZE 64000
#define BUFFERCMD 1024
#define NETWERKPOORT 2200
#define MAX_CLI 10
//Debug mode 0 off, 1 speedtest, 2 error debug
#define DEBUG 0
//Defines return values
#define STUK -1
#define MOOI 0

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
#define STATUS_MKDIR 303 //aanvraag voor een create directory
#define STATUS_SYNC 304 //geeft aan dat hij klaar is voor synchronisatie van de ander
#define STATUS_DELETE 305 //Geeft aan dat client een bestand op de server wilt verwijderen
#define STATUS_RENAME 306 //Geeft aan dat de client een bestand of folder wilt hernoemen

// 4xx Server naar client requests
#define STATUS_OLD 401 //Server geeft aan dat file op server ouder is.
#define STATUS_NEW 402 //Server geeft aan dat file op server nieuwer is.
#define STATUS_SAME 403 //Server geeft aan dat de file hetzelfde is als die op de server.
#define STATUS_AUTHOK 404 //Server verteld de client dat de authenticatie gelukt is.
#define STATUS_AUTHFAIL 405 //Server verteld de client dat de authenticatie is mislukt.
// Login defines
#define LOGINATTEMPTS 3 //Binnen dit aantal moet de gebruiker het wachtwoord goed raden. Anders wordt de verbinding verbroken.
// Password protection
#define FIXEDSALT "Sorbet"

//De client vult hier true in. De server false. Dit is nodig voor sommige functies
int IS_CLIENT;

//File Functions
int BestaatDeFile(char* fileName);
int OpenBestand(char* bestandsnaam);
int changeModTime(char *bestandsnaam, int time);
int modifiedTime(char *bestandsnaam);

int deleteFile(SSL* ssl, char *bestandsnaam, char* fileOrDir);
int renameFile(SSL* ssl, char* oldName, char* newName);

//Communication Client Server
int CreateFolder(SSL* ssl, char* bestandsnaam);
int FileTransferSend(SSL* ssl, char* bestandsnaam);
int FileTransferReceive(SSL* ssl, char* bestandsnaam, int time);
int waitForOk(SSL* ssl);
int ConnectRefused(SSL* ssl);


//String Editing
int transform(char *text, char** to);
int transformWith(char *text, char** to, char *delimit);
char *toString(int number);
void getEOF(char *to);
char* fixServerBestand(SSL* ssl, char* bestandsnaam);

//Input user
char* getInput(int max);
char* invoerCommands(char* tekstVoor, int aantalChars);

//Switch functions
int switchResult(SSL* ssl, char* buffer);
int sendPacket(SSL* ssl, int packet, ...);
int loopOverFiles(SSL* ssl, char *path);
int ModifyCheckFile(SSL* ssl, char* bestandsnaam);

//Visual presentation
void printStart(void);
void printArray(int length, char *array[]);

//Password Hashing
int hashPassword(char *password, char *salt, char to[]);
int randomSalt(char *salt, int aantalBytes);
int convertHashToString(char *stringHash, unsigned char hash[]);

//DB - Will be edited
int callback(void *NotUsed, int argc, char **argv, char **azColName);
int connectDB();
int initDB();
int createUser(char **args, int amount); //server only
int removeUser(char **args, int amount); //server only
int userExists(char* name);
int closeDB();
char* getSalt(char *name); //server only
void printRes(sqlite3_stmt *res);
int writePasswordToLocalDB(int rc);
sqlite3_stmt* selectQuery(char *query);

//Server Only
int ModifyCheckServer(SSL* ssl, char* bestandsnaam, char* timeleft);

//Struct om username/IP van client in op te slaan
typedef struct clientsinfo{
    struct sockaddr_in client;
    char* username;
} clientsinfo;

clientsinfo *clients;
