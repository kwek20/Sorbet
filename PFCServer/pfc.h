/* 
 * File:   pfc.h
 * Author: Bartjan Zondag & Kevin Rosendaal
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


#define BUFFERSIZE 4096
#define NETWERKPOORT 2200

#define STUK -1
#define MOOI 0

#define MAX_CLI 10

// 1xx OK en gerelateerde statussen
#define STATUS_OK  100 // OK (bevestigingscode)
#define STATUS_EOF 101 // End-of-file
// 2xx Errors en gerelateerde statussen
#define STATUS_CL  200 // Verbinding verbroken
#define STATUS_FC  201 // Bestand corrupt
#define STATUS_WPN 202 // Verkeerde packet nummer
#define STATUS_CNA 203 // Verbinding niet toegestaan
// 3xx Client naar server requests
#define STATUS_CR  300 // Client vraagt bestandsoverdracht aan
#define STATUS_MODCHK  301 // Client vraagt aan de server of er nieuwe/gewijzigde bestanden zijn.
// 4xx Server naar client requests
#define STATUS_OLD 401 //Server geeft aan dat file op server ouder is
#define STATUS_NEW 402 //Server geeft aan dat file op server nieuwer is


int pfcClient(char** argv);
int ServerGegevens(char* ip);
int BestaatDeFile(char* fileName);
int ConnectNaarServer(int* sockfd);
int FileTransferSend(int* sockfd, char* bestandsnaam);
int FileTransferReceive(int* sockfd, char* bestandsnaam);
int OpenBestand(char* bestandsnaam);
int ModifyCheckServer(int* sockfd, char *bestandsnaam, char* timeleft);
int ModifyCheckClient(int* sockfd, char* bestandsnaam);

int transform(char *text, char** to);
int switchResult(int* sockfd, char* buffer);
int sendPacket(int fd, int packet, ...);

int modifiedTime(char *bestandsnaam);
char *toString(int number);
void getEOF(char *to);

int waitForOk(int fd);