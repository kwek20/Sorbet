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
#define OK  100 // OK (bevestigingscode)
#define EOF 101 // End-of-file
// 2xx Errors en gerelateerde statussen
#define CL  200 // Verbinding verbroken
#define FC  201 // Bestand corrupt
#define WPN 202 // Verkeerde packet nummer
#define CNA 203 // Verbinding niet toegestaan
// 3xx Client naar server requests
#define CR  300 // Client vraagt bestandsoverdracht aan
#define CC  301 // Client vraagt aan de server of er nieuwe/gewijzigde bestanden zijn.
// 4xx Server naar client requests
// ..
