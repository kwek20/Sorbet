/* 
 * File:   pfcclient.h
 * Author: Bartjan Zondag & Kevin Rosendaal
 *
 * Created on November 20, 2013, 11:26 AM
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

#define BUFFERGROOTE 4096
#define NETWERKPOORT 2200

// 1xx OK en gerelateerde statussen
#define OK  "100" // OK (bevestigingscode)
#define EOF "101" // End-of-file
// 2xx Errors en gerelateerde statussen
#define CL  "200" // Verbinding verbroken
#define FC  "201" // Bestand corrupt
#define WPN "202" // Verkeerde packet nummer
#define CNA "203" // Verbinding niet toegestaan
// 3xx Client naar server requests
#define CR  "300" // Client vraagt bestandsoverdracht aan
#define CC  "301" // Client vraagt aan de server of er nieuwe/gewijzigde bestanden zijn.
// 4xx Server naar client requests


int pfcClient(char** argv);
int ServerGegevens(char* ip);
int BestaatDeFile(char* fileName);
int ConnectNaarServer();
int FileNaarServer();
int OpenBestand(char* bestandsnaam);