/* 
 * File:   pfcclient.h
 * Author: Dave van Hooren
 *
 * Created on November 22, 2013, 11:04 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <memory.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>

#define PORT 2200
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


